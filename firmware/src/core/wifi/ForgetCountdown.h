#ifndef FORGETCOUNTDOWN_H
#define FORGETCOUNTDOWN_H

// Issue #60: holding RIGHT while the orbs power up forgets the saved WiFi.
// It used to happen at once, on one read of the pin, so a thumb resting on the
// button while plugging the cable in wiped the network. Now the middle orb
// counts down "3, 2, 1" over 5 seconds and the WiFi is forgotten only if the
// button is still down at the end; letting go before that keeps it. No
// Arduino types, so the host tests can build it (test/test_forgetwifi).
//
// On the rig (Scott, 2026-11-19) a 3-second "3, 2, 1" on the status orb 4 went
// by before he found it. He asked for orb 3, then for three numbers that last
// 5 seconds, so each number stays up about 1.7 s.
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

constexpr uint32_t kCountMs = 5000; // the whole count
constexpr int kNumbers = 3; // "3, 2, 1", each shown for kCountMs / kNumbers
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

    // The number on the orb: 3, then 2, then 1, then 0 at the end. Rounded
    // up, so the 3 shows from the first moment and the 1 until the end.
    int numberLeft(uint32_t nowMs) const {
        const uint32_t gone = nowMs - m_start;
        if (gone >= kCountMs) {
            return 0;
        }
        return static_cast<int>(((kCountMs - gone) * kNumbers + kCountMs - 1) / kCountMs);
    }

  private:
    uint32_t m_start;
    bool m_up = false; // seen up, maybe only bouncing
    uint32_t m_upSince = 0;
};

} // namespace forgetwifi

#endif
