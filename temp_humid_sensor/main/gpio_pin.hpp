// Typed GPIO wrappers — make pin misuse a *compile-time* error.
//
// The ESP-IDF C API takes a flat `gpio_num_t`, so `gpio_set_level(GPIO_NUM_36, x)`
// compiles and only fails at runtime (GPIO34-39 are input-only). Here the pin
// number lives in the *type* (a template parameter), so:
//
//   - OutputPin<GPIO_NUM_36>  -> build fails: input-only pin can't output.
//   - An InputPin has no set() method at all, so you can't drive it by mistake.
//
// This is the C++ way to get the "TypeScript underlines the bug before you run
// it" experience: clangd shows the static_assert / missing-method error live.

#pragma once

extern "C" {
#include "driver/gpio.h"
#include "hal/gpio_types.h"
}

namespace pin {

// An output-capable pin. Instantiating it with an input-only or invalid GPIO is
// a hard compile error.
template <gpio_num_t PIN>
class OutputPin {
    static_assert(GPIO_IS_VALID_OUTPUT_GPIO(PIN),
        "OutputPin: this GPIO cannot drive an output. "
        "On the ESP32, GPIO34-39 are input-only.");

public:
    OutputPin() { gpio_set_direction(PIN, GPIO_MODE_OUTPUT); }

    void set(int level) const { gpio_set_level(PIN, level); }
    void high() const { gpio_set_level(PIN, 1); }
    void low()  const { gpio_set_level(PIN, 0); }

    static constexpr gpio_num_t num() { return PIN; }
};

// A readable pin. Any valid GPIO can be an input, so only validity is asserted.
// Note there is deliberately no set()/high()/low() here — you cannot drive it.
template <gpio_num_t PIN>
class InputPin {
    static_assert(GPIO_IS_VALID_GPIO(PIN), "InputPin: not a valid GPIO number.");

public:
    explicit InputPin(gpio_pull_mode_t pull = GPIO_FLOATING) {
        gpio_set_direction(PIN, GPIO_MODE_INPUT);
        if (pull != GPIO_FLOATING) gpio_set_pull_mode(PIN, pull);
    }

    int get() const { return gpio_get_level(PIN); }

    static constexpr gpio_num_t num() { return PIN; }
};

// A single-wire bidirectional pin (open-drain style), needed for protocols like
// the DHT22 where the same line is driven low then read. Requires output
// capability, so input-only pins are rejected at compile time.
template <gpio_num_t PIN>
class OpenDrainPin {
    static_assert(GPIO_IS_VALID_OUTPUT_GPIO(PIN),
        "OpenDrainPin: this GPIO cannot drive an output. "
        "On the ESP32, GPIO34-39 are input-only, so a bidirectional "
        "single-wire bus (e.g. DHT22) cannot use them.");

public:
    explicit OpenDrainPin(gpio_pull_mode_t pull = GPIO_PULLUP_ONLY) {
        gpio_set_pull_mode(PIN, pull);
    }

    // Drive the line: as_output() then low()/high().
    void as_output() const { gpio_set_direction(PIN, GPIO_MODE_OUTPUT); }
    void low()  const { gpio_set_level(PIN, 0); }
    void high() const { gpio_set_level(PIN, 1); }

    // Release the line back to the bus and read it.
    void as_input() const { gpio_set_direction(PIN, GPIO_MODE_INPUT); }
    int  get() const { return gpio_get_level(PIN); }

    static constexpr gpio_num_t num() { return PIN; }
};

}  // namespace pin
