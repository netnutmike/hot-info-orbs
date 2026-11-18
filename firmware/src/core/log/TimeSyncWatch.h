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

// The system clock holds a real time: SNTP has set it. Before that it counts
// from 1970, and the orb showed that as a time of day (2:26 on the rig,
// 2026-11-18). Same line as the device log's clockSet().
inline bool timeValid(int64_t utc) {
    return utc > 1700000000;
}

// A sync since start that is not overdue: the internet was there lately.
// Not the same as timeValid(): a restart keeps the clock (rig, 2026-11-18),
// so a valid time after a restart says nothing about the internet.
inline bool syncCurrent(uint32_t upS, uint32_t syncs, uint32_t lastSyncUpS) {
    return syncs > 0 && upS - lastSyncUpS < kSyncOverdueS;
}

// While there is no current sync (syncCurrent), ask SNTP again every kKickMs,
// and at once when WiFi comes back. lwIP's own retry backs off from 15 s to 150 s, so a boot
// with no internet took 3.5 min to show the time once it returned (rig,
// 2026-11-18). A kick (sntp_restart) only starts a request: nothing waits.
// Once a sync is current, no kicks: the 15 min sync takes over.
constexpr uint32_t kKickMs = 15000;
class SyncKick {
public:
    // Every loop pass. synced: syncCurrent(). true = restart SNTP now.
    bool step(uint32_t nowMs, bool synced, bool linkUp) {
        if (!m_started) {
            m_started = true; // SNTP was started at boot: the interval runs from here
            m_last = nowMs;
        }
        const bool back = linkUp && !m_up;
        m_up = linkUp;
        if (synced || !linkUp) {
            return false;
        }
        if (back || nowMs - m_last >= kKickMs) { // unsigned: right across the millis() wrap
            m_last = nowMs;
            return true;
        }
        return false;
    }

private:
    bool m_started = false;
    bool m_up = true; // pages only run once start-up has joined WiFi
    uint32_t m_last = 0;
};

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
