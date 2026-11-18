#pragma once
// Device-log lines for the time sync (issue #52). The sync runs in the
// background (ESP32 SNTP) and never holds the loop; between syncs the clock
// runs on the ESP32's own crystal. These lines only say when the sync has not
// worked for a while, and when it works again.
//
// Pure, so it builds on the host:  pio test -e native -f test_timesync
#include <cstdint>
#include <string>

namespace devlog {

constexpr uint32_t kSyncIntervalS = 15 * 60; // GlobalTime asks SNTP for this
constexpr uint32_t kFirstSyncWarnS = 120; // no time this long after start is news
constexpr uint32_t kSyncOverdueS = 2 * kSyncIntervalS + 5 * 60; // two missed syncs, and some

// Call every loop pass. upS: seconds of uptime; syncs: syncs so far;
// lastSyncUpS: uptime of the last one (ignored while syncs is 0).
class TimeSyncWatch {
public:
    std::string step(uint32_t upS, uint32_t syncs, uint32_t lastSyncUpS) {
        if (syncs != m_syncs) {
            std::string out;
            if (m_warned && m_syncs == 0) {
                out = "time sync ok, first one " + std::to_string(upS) + " s after start";
            } else if (m_warned) {
                out = "time sync ok again after " + std::to_string((upS - m_lastSyncUpS) / 60) + " min without one";
            }
            m_syncs = syncs;
            m_lastSyncUpS = lastSyncUpS;
            m_warned = false;
            return out;
        }
        if (m_warned) {
            return "";
        }
        if (syncs == 0 && upS >= kFirstSyncWarnS) {
            m_warned = true;
            return "no time sync yet, " + std::to_string(upS) + " s after start";
        }
        if (syncs > 0 && upS - lastSyncUpS >= kSyncOverdueS) {
            m_warned = true;
            return "no time sync for " + std::to_string((upS - lastSyncUpS) / 60) + " min";
        }
        return "";
    }

private:
    uint32_t m_syncs{0};
    uint32_t m_lastSyncUpS{0};
    bool m_warned{false};
};

} // namespace devlog
