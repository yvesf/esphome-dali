#include "esphome_light.h"

namespace esphome {
namespace dali {

static const char *const TAG = "dali.output";

void Output::setup_state(light::LightState *state) {
    state->set_default_transition_length(0);
    state->set_gamma_correct(1.0f);

    auto err = libdali::SelectDimmingCurve(this, 0);
    if (err) {
        ESP_LOGE(TAG, "'%s' Failed to select dimming curve linear: %s", state->get_name().c_str(), err);
    }

    auto r = libdali::QueryActualLevel(this);
    if (r) {
        if (r.value() == 0) {
            ESP_LOGD(TAG, "'%s' Lamp is off, leave off.", state->get_name().c_str());
        } else {
            auto target_brightness = 1.0f/254.0f * float(r.value());
            this->restore_brightness = target_brightness;
            // The "State" in initial state is restored and overriden by "Restore" mode.
            // Hack: Change the restore mode. Change from DEFAULT_OFF to LIGHT_ALWAYS_ON.
            // Custom code in write_state.
            state->set_restore_mode(esphome::light::LIGHT_ALWAYS_ON);
            ESP_LOGD(TAG, "'%s' Lamp is on, set to %02x => %f", state->get_name().c_str(), r.value(), target_brightness);
        }
    } else {
        ESP_LOGE(TAG, "'%s' Failed to query actual level: %s", state->get_name().c_str(), err);
    }
}

light::LightTraits Output::get_traits() {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
}

void Output::write_state(light::LightState *state) {
    if (this->restore_brightness.has_value()) {
        ESP_LOGD(TAG, "'%s' Restore", state->get_name().c_str());
        auto call = state->make_call();
        call.set_brightness(this->restore_brightness.value());
        call.set_state(true);
        this->restore_brightness.reset();
        call.perform();
    } else {
        ESP_LOGD(TAG, "'%s' NOT Restore", state->get_name().c_str());
    }
    bool on;
    float brightness;
    state->current_values_as_brightness(&brightness);
    state->current_values_as_binary(&on);
    auto targetBrightness = uint8_t(254.0f * float(brightness));

    ESP_LOGI(TAG, "'%s' write_state: On: %d Brightness: %f, Dali value: %x", state->get_name().c_str(), on, brightness, targetBrightness);
    if (!on) {
        targetBrightness = 0;
    }

    auto err = libdali::DirectArc(this, targetBrightness);
    if (err) {
        ESP_LOGE(TAG, "'%s' Direct Arc Control failed: %s", state->get_name().c_str(), err);
    }

    auto r = libdali::QueryActualLevel(this);
    if (r) {
        ESP_LOGD(TAG, "'%s' QueryActualLevel Result: %02x", state->get_name().c_str(), r.value());
    }
}


}  // namespace lw14
}  // namespace esphome
