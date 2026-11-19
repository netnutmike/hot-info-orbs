#include "Button.h"
#include "PressPolicy.h"
#include "config_helper.h"

/**
 * After calling begin() make sure to attach an interrupt handler in main.cpp that will call isrButtonChange()
 */
Button::Button(uint8_t pin)
    : m_pin(pin), m_lastPinLevelChange(0),
      m_pinLevel(RELEASED_LEVEL), m_state(BTN_NOTHING),
      m_hasChanged(false) {}

void Button::begin() {
    pinMode(m_pin, BUTTON_MODE);
}

void Button::isrButtonChange() {
    if (press::bounce(millis(), m_lastPinLevelChange, DEBOUNCE_TIME)) {
        return;
    }

    bool newPinLevel = digitalRead(m_pin);
    if (newPinLevel != m_pinLevel) {
        m_pinLevel = newPinLevel;
        m_lastPinLevelChange = millis();
        if (m_pinLevel == RELEASED_LEVEL) {
            // Let go: a tap or a hold, by how long it was down
            m_state = press::classify(millis() - m_pressedSince, LONG_PRESS_TIME) == press::Kind::Hold ? BTN_HOLD : BTN_TAP;
        } else {
            // Start button press
            m_pressedSince = millis();
            m_state = BTN_NOTHING;
        }
        m_hasChanged = true;
    }
}

bool Button::has_changed() {
    if (m_hasChanged == true) {
        m_hasChanged = false;
        return true;
    }
    return false;
}

ButtonState Button::getState() {
    if (has_changed()) {
        return m_state;
    } else {
        return BTN_NOTHING;
    }
}

bool Button::heldFor(unsigned long ms) {
    // The pin itself as well as what the interrupt last saw: a release edge
    // lost to the debounce window must not read as a finger still down
    return m_pinLevel == PRESSED_LEVEL && digitalRead(m_pin) == PRESSED_LEVEL && millis() - m_pressedSince >= ms;
}
