#ifndef PAGELOCK_H
#define PAGELOCK_H

// Issue #58: hold left and right together for 2 seconds to keep the orbs on
// the page they show (the rotation stops); the same hold again lets it go.
// No Arduino types, so the host tests can build it (test/test_pagelock).
//
// Fed the raw "down right now" of the two side buttons every pass of the
// button check. It answers two things:
//   event:   the hold just locked or unlocked;
//   swallow: these two buttons are part of a hold, so their presses must do
//            nothing else. That lasts until both are up, and includes the
//            pass that sees them up, so the release (which the Button class
//            reports as a press of its own) never changes the page.
//
// Holding RIGHT while the orbs power up wipes the WiFi settings
// (WifiWidget.cpp). A hold carried over from power-up must never lock, so
// nothing counts until both buttons have been seen up once.
//
// A restart clears the lock on purpose: an orb never comes back from a power
// cut stuck on one page.

#include <cstdint>

namespace pagelock {

constexpr uint32_t kHoldMs = 2000; // same as the middle hold for the info page
constexpr uint32_t kNoteMs = 1500; // how long "Locked" / "Unlocked" stays up

enum class Event { None, Locked, Unlocked };

struct Step {
    Event event = Event::None;
    bool swallow = false;
};

class PageLock {
  public:
    Step step(uint32_t nowMs, bool leftDown, bool rightDown) {
        Step out;
        if (!leftDown && !rightDown) {
            out.swallow = m_swallow; // the release pass
            m_swallow = false;
            m_both = false;
            m_fired = false;
            m_armed = true;
            return out;
        }
        if (!m_armed) {
            return out;
        }
        if (leftDown && rightDown) {
            m_swallow = true;
            if (!m_both) {
                m_both = true;
                m_since = nowMs;
            }
            if (!m_fired && nowMs - m_since >= kHoldMs) {
                m_fired = true; // one event per hold, however long
                m_locked = !m_locked;
                m_noteOn = true;
                m_noteAt = nowMs;
                out.event = m_locked ? Event::Locked : Event::Unlocked;
            }
        } else {
            m_both = false; // one let go: the 2 seconds start again if it comes back
        }
        out.swallow = m_swallow;
        return out;
    }

    bool locked() const { return m_locked; }

    // The "Locked" / "Unlocked" note: showing since the last event, and true
    // from noteEnded() exactly once, when it is time to take it down.
    bool noteShowing() const { return m_noteOn; }
    bool noteEnded(uint32_t nowMs) {
        if (m_noteOn && nowMs - m_noteAt >= kNoteMs) {
            m_noteOn = false;
            return true;
        }
        return false;
    }

  private:
    bool m_armed = false; // both seen up since start
    bool m_both = false; // both down now, in a hold that counts
    uint32_t m_since = 0; // when both went down
    bool m_fired = false; // this hold has already locked or unlocked
    bool m_swallow = false; // a hold is under way: eat these buttons' presses
    bool m_locked = false;
    bool m_noteOn = false;
    uint32_t m_noteAt = 0;
};

} // namespace pagelock

#endif
