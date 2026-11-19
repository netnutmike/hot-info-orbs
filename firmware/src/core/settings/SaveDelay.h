#ifndef SAVEDELAY_H
#define SAVEDELAY_H

// Issue #60: a choice made with a button (the clock face) is saved where the
// settings page saves it, so the two never disagree and it survives a restart.
// No Arduino types, so the host tests can build it (test/test_savedelay).
//
// The write waits until the button has been left alone for kDelayMs, so
// tapping through the faces writes the flash once, not once per tap.
//
// REJECTED: writing on every tap. Flash wear is small either way, but each NVS
// write can stall the loop for milliseconds, in the middle of a run of taps.
// REJECTED: writing only at the next restart. A power cut is the common restart
// and gives no warning.

#include <cstdint>

namespace savedelay {

constexpr uint32_t kDelayMs = 5000;

class SaveDelay {
  public:
    // Something changed: the write is owed, kDelayMs from now
    void changed(uint32_t nowMs) {
        m_owed = true;
        m_at = nowMs;
    }

    // True once, on the pass where the write is due. Subtraction, so it holds
    // across the millisecond counter wrapping.
    bool due(uint32_t nowMs) {
        if (m_owed && nowMs - m_at >= kDelayMs) {
            m_owed = false;
            return true;
        }
        return false;
    }

    bool owed() const { return m_owed; }

  private:
    bool m_owed = false;
    uint32_t m_at = 0;
};

} // namespace savedelay

#endif
