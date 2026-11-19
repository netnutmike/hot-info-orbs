#ifndef PRESSPOLICY_H
#define PRESSPOLICY_H

// Issue #60: a button press is a tap or a hold, nothing in between. No Arduino
// types, so the host tests can build it (test/test_presspolicy).
//
// A release under 2 seconds is a tap, at 2 seconds or more a hold. Each button
// has one job per gesture:
//   Left tap   previous page        Left hold    nothing (with Right: page lock, PageLock.h)
//   Right tap  next page            Right hold   nothing (with Left: page lock)
//   Middle tap the page's action    Middle hold  info page (InfoPagePolicy.h)
//
// REJECTED: keeping the half-second "medium" press. Every release from 0.5 to
// 2 s was medium, and only Clock (12/24 h) and Parqet (timeframe) used it, so a
// slightly slow tap on any other button did nothing at all.
//
// The 2 s is the same number heldFor() is asked about for the info page and
// the page lock, so a press that opened one of them always ends as a hold.

#include <cstdint>

namespace press {

enum class Kind { Tap, Hold };

// How long the button was down decides it, at release
inline Kind classify(uint32_t downMs, uint32_t holdMs) {
    return downMs >= holdMs ? Kind::Hold : Kind::Tap;
}

// An edge this soon after the last one the button took is contact bounce, and
// is dropped. Subtraction, so it holds across the millisecond counter wrapping.
inline bool bounce(uint32_t nowMs, uint32_t lastEdgeMs, uint32_t debounceMs) {
    return nowMs - lastEdgeMs < debounceMs;
}

} // namespace press

#endif
