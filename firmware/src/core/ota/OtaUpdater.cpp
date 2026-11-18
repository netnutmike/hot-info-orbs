#include "OtaUpdater.h"
#include "DeviceLogStore.h"
#include "UpdateCheck.h"
#include "UploadResult.h"
#include "Utils.h"
#include "build_id.h"
#include "config_helper.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <esp_ota_ops.h>
#include <functional>

static WebServer s_server(80);

// The Arduino core marks a freshly updated image good before setup() runs, so
// an image that boots and then cannot get back on the network would be kept.
// Overriding this hook defers that until begin(): only an image that has
// reached WiFi and is listening for the next update is confirmed. One that
// resets before then is rolled back to the previous slot by the bootloader.
extern "C" bool verifyRollbackLater() {
    return true;
}

// Same warning as the settings page: both pages are open when there is no password
#ifdef OTA_PASSWORD
    #define NO_PASSWORD_WARNING ""
#else
    #define NO_PASSWORD_WARNING "<p>&#9888; No password is set, so anyone on this network can install firmware " \
                                "and change the settings. Set OTA_PASSWORD in secrets.h.</p>"
#endif

// The two release links are plain HTML on purpose: they work with scripts off
// and with no answer from the version check (UpdateCheck.h). The page is closed
// in the GET handler, after the check's script.
static const char *uploadPage =
    "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width'>"
    "<title>Info Orbs update</title></head><body style='font-family:sans-serif'>"
    "<h2>Info Orbs firmware update</h2>"
    "<p>Running: " FIRMWARE_VERSION ", commit " BUILD_COMMIT ", built " BUILD_TIME " for " BUILD_ENV UPDATE_CHECK_MARKER_UPDATE "</p>"
    NO_PASSWORD_WARNING
    "<p>Newest release: <a href='" UPDATE_CHECK_FILE_URL "'>download firmware.bin</a> &middot; "
    "<a href='" UPDATE_CHECK_NOTES_URL "'>release notes</a></p>"
    "<form method='POST' action='/update' enctype='multipart/form-data'>"
    "<input type='file' name='firmware' accept='.bin' required> "
    "<input type='submit' value='Upload'></form>"
    "<p>Choose the <code>firmware.bin</code> you downloaded, or your own build's "
    "<code>.pio/build/&lt;env&gt;/firmware.bin</code>. The orbs restart when it is done.</p>"
    "<p><a href='/settings'>Settings</a></p>";

// POST /update takes multipart form uploads only (#55). The core's own
// on(uri, method, fn, upload) handler also accepts a raw body (any POST that is
// not a form, even an empty one) and calls the same upload function for it,
// which reads server.upload(): a null pointer on that path, so the orbs
// crashed. Here the upload arrives as an argument and raw bodies are refused,
// so such a POST reaches the finish step with no bytes: "No file chosen".
class UpdatePostHandler : public RequestHandler {
  public:
    UpdatePostHandler(std::function<void()> finish, std::function<void(HTTPUpload &)> chunk)
        : m_finish(finish), m_chunk(chunk) {}
    bool canHandle(HTTPMethod method, String uri) override { return method == HTTP_POST && uri == "/update"; }
    bool canUpload(String uri) override { return uri == "/update"; }
    bool canRaw(String uri) override { return false; }
    bool handle(WebServer &server, HTTPMethod method, String uri) override {
        if (!canHandle(method, uri)) {
            return false;
        }
        m_finish();
        return true;
    }
    void upload(WebServer &server, String uri, HTTPUpload &upload) override {
        if (canUpload(uri)) {
            m_chunk(upload);
        }
    }

  private:
    std::function<void()> m_finish;
    std::function<void(HTTPUpload &)> m_chunk;
};

OtaUpdater::OtaUpdater(ScreenManager &manager) : m_manager(manager) {}

void OtaUpdater::begin() {
    if (m_started) {
        return;
    }
    setupArduinoOta(); // starts mDNS with OTA_HOSTNAME
    setupWebUpdate();
    m_started = true;
    esp_ota_mark_app_valid_cancel_rollback(); // harmless when not pending (e.g. after a USB flash)
    Serial.printf("OTA ready: http://%s.local/update (%s)\n", OTA_HOSTNAME, WiFi.localIP().toString().c_str());
#ifndef OTA_PASSWORD
    Serial.println("OTA has no password - anyone on this network can flash the orbs. Set OTA_PASSWORD in secrets.h.");
#endif
}

WebServer &OtaUpdater::server() {
    return s_server;
}

bool OtaUpdater::authorized() {
#ifdef OTA_PASSWORD
    if (!s_server.authenticate("admin", OTA_PASSWORD)) {
        s_server.requestAuthentication();
        return false;
    }
#endif
    return true;
}

void OtaUpdater::handle() {
    if (!m_started) {
        return;
    }
    ArduinoOTA.handle();
    s_server.handleClient();
}

void OtaUpdater::setupArduinoOta() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
#ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
#endif
    ArduinoOTA.onStart([this]() {
        m_updating = true;
        m_lastPercent = -1;
        drawStatus("Updating", "do not unplug", TFT_ORANGE);
    });
    ArduinoOTA.onProgress([this](unsigned int done, unsigned int total) { drawProgress(done, total); });
    ArduinoOTA.onEnd([this]() {
        drawStatus("Update OK", "restarting", TFT_GREEN);
        DeviceLog::beforeRestart("firmware update from the build tool"); // ArduinoOTA restarts next
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        Serial.printf("OTA error %u\n", error);
        drawStatus("Update failed", "old firmware kept", TFT_RED);
        delay(3000);
        DeviceLog::beforeRestart("firmware update from the build tool failed");
        ESP.restart(); // screens are in an unknown state; a restart redraws everything
    });
    ArduinoOTA.begin();
}


void OtaUpdater::setupWebUpdate() {

    s_server.on("/update", HTTP_GET, [this]() {
        if (!authorized()) {
            return;
        }
        String page = uploadPage;
        page += UPDATE_CHECK_SCRIPT;
        page += "</body></html>";
        s_server.send(200, "text/html", page);
    });

    s_server.addHandler(new UpdatePostHandler(
        // Runs after the upload has finished
        [this]() {
            if (!authorized()) {
                return;
            }
            const ota::Outcome o = ota::outcome(m_uploadBytes, Update.hasError());
            const std::string page = ota::resultPage(o, Update.errorString());
            const bool ok = o == ota::Outcome::Ok;
            m_uploadBytes = 0;
            s_server.send(o == ota::Outcome::Failed ? 500 : 200, "text/html", page.c_str());
            if (!ota::restartAfter(o)) {
                // No file: Update.begin() was never called and the screens were never touched
                Serial.println("OTA upload: no file chosen, nothing done");
                return;
            }
            drawStatus(ok ? "Update OK" : "Update failed", ok ? "restarting" : "old firmware kept", ok ? TFT_GREEN : TFT_RED);
            delay(1000);
            DeviceLog::beforeRestart(ok ? "firmware update" : "firmware update failed");
            ESP.restart();
        },
        // Runs once per chunk while the upload streams in
        [this](HTTPUpload &upload) {
#ifdef OTA_PASSWORD
            if (!s_server.authenticate("admin", OTA_PASSWORD)) {
                return;
            }
#endif
            if (upload.status == UPLOAD_FILE_START) {
                // Nothing starts until bytes arrive: no file chosen (and an empty
                // file) sends none, and must not take over the screens (#55)
                Serial.printf("OTA upload: %s\n", upload.filename.c_str());
                m_uploadBytes = 0;
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (upload.currentSize == 0) {
                    return;
                }
                if (m_uploadBytes == 0) {
                    m_updating = true;
                    m_lastPercent = -1;
                    drawStatus("Updating", "do not unplug", TFT_ORANGE);
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                        Update.printError(Serial);
                    }
                }
                m_uploadBytes += upload.currentSize;
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
                // Total size is unknown for a browser upload; show kilobytes
                if (upload.totalSize / 65536 != (upload.totalSize - upload.currentSize) / 65536) {
                    drawStatus("Updating", String(upload.totalSize / 1024) + " KB", TFT_ORANGE);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (m_uploadBytes == 0) {
                    return; // never began
                }
                // true: accept the size as whatever arrived. Only here is the new slot marked bootable.
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                if (m_uploadBytes > 0) {
                    Update.abort();
                }
                m_updating = false;
                Serial.println("OTA upload aborted");
            }
        }));

    s_server.begin();
    MDNS.addService("http", "tcp", 80);
}

void OtaUpdater::drawStatus(const String &line1, const String &line2, uint32_t color) {
    m_manager.selectScreen(2);
    m_manager.clearScreen();
    // Otherwise it inherits the current widget's font: on the clock that is
    // DSEG, which has almost no letters. Widgets set their own font per draw.
    m_manager.setFont(DEFAULT_FONT);
    m_manager.setFontColor(color);
    m_manager.drawCenterString(line1, ScreenCenterX, ScreenCenterY - 20, 22);
    m_manager.setFontColor(TFT_WHITE);
    m_manager.drawCenterString(line2, ScreenCenterX, ScreenCenterY + 20, 16);
}

void OtaUpdater::drawProgress(size_t done, size_t total) {
    int percent = total ? (done * 100) / total : 0;
    if (percent / 5 == m_lastPercent / 5) {
        return; // redraw every 5% - each redraw slows the transfer
    }
    m_lastPercent = percent;
    drawStatus("Updating", String(percent) + "%", TFT_ORANGE);
}
