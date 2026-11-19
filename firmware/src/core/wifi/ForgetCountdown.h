#ifndef FORGETCOUNTDOWN_H
#define FORGETCOUNTDOWN_H

// Issue #60: holding RIGHT while the orbs power up forgets the saved WiFi.
// It used to happen at once, on one read of the pin, so a thumb resting on the
// button while plugging the cable in wiped the network. Now the middle orb
// counts down "5, 4, 3, 2, 1" and the WiFi is forgotten only if the button is
// still down at the end; letting go before that keeps it.
//
// Five seconds, not three, and on orb 3, not the status orb 4: on the rig
// (Scott, 2026-11-19) three seconds on orb 4 went by before he found it. No Arduino types, so the
// host tests can build it (test/test_forgetwifi).
//
// REJECTED: moving it to the info page as a confirmed action. Any button
// closes the info page and does nothing else (InfoPagePolicy.h); a confirm
// there would break that rule, and an orb whose WiFi is wrong may never get
// far enough to show the info page anyway.
//
// A let-go has to last kLetGoMs to count, so contact bounce on a finger
// that is still pressing does not cancel the countdown.

#include <cstdint>

namespace forgetwifi {

constexpr uint32_t kCountMs = 5000; // "5, 4, 3, 2, 1"
constexpr uint32_t kLetGoMs = 50; // longer than the 35 ms button debounce

enum class State {
    Counting, // still holding: keep showing the count
    Kept, // let go before the end: the WiFi stays
    Forget // held to the end: forget the WiFi
};

class Countdown {
  public:
    explicit Countdown(uint32_t startMs) : m_start(startMs) {}

    // Every few milliseconds, with the button's level read now
    State step(uint32_t nowMs, bool down) {
        if (down) {
            m_up = false;
            return nowMs - m_start >= kCountMs ? State::Forget : State::Counting;
        }
        if (!m_up) {
            m_up = true;
            m_upSince = nowMs;
        }
        return nowMs - m_upSince >= kLetGoMs ? State::Kept : State::Counting;
    }

    // The number on the orb: 5, 4, 3, 2, 1, then 0 at the end
    int secondsLeft(uint32_t nowMs) const {
        const uint32_t gone = nowMs - m_start;
        if (gone >= kCountMs) {
            return 0;
        }
        return static_cast<int>((kCountMs - gone + 999) / 1000);
    }

  private:
    uint32_t m_start;
    bool m_up = false; // seen up, maybe only bouncing
    uint32_t m_upSince = 0;
};

} // namespace forgetwifi

#endif
