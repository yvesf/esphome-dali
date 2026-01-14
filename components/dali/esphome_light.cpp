#include "esphome_light.h"

namespace esphome {
namespace dali {

static const char *const TAG = "dali.output";

void Output::setup_state(light::LightState *state) {
    auto address = libdali::Address::from_short_address(this->short_address);

    ESP_LOGD(TAG, "'%s' Dali Address is %d => %d", state->get_object_id().c_str(), this->short_address, address.command());

    state->set_default_transition_length(0);
    state->set_gamma_correct(1.0f);

    auto err = libdali::SelectDimmingCurve(this->bus, address, 0);
    if (err) {
        ESP_LOGE(TAG, "'%s' Failed to select dimming curve linear: %s", state->get_object_id().c_str(), err.text());
    }

    auto r = libdali::QueryActualLevel(this->bus, address);
    if (r.error()) {
        ESP_LOGE(TAG, "'%s' Failed to query actual level: %s", state->get_object_id().c_str(), r.error().text());
        return;
    }
    if (r.value() == 0) {
        ESP_LOGD(TAG, "'%s' Lamp is off, leave off.", state->get_object_id().c_str());
        state->set_restore_mode(esphome::light::LIGHT_ALWAYS_OFF);
    } else {
        auto target_brightness = 1.0f/254.0f * float(r.value());
        this->restore_brightness = target_brightness;
        // The "State" in initial state is restored and overriden by "Restore" mode.
        // Hack: Change the restore mode. Change from DEFAULT_OFF to LIGHT_ALWAYS_ON.
        // Custom code in write_state handles this value.
        state->set_restore_mode(esphome::light::LIGHT_ALWAYS_ON);
        ESP_LOGD(TAG, "'%s' Lamp is on, set to actualLevel=%02x target=%f", state->get_object_id().c_str(), r.value(), target_brightness);
    }
}

light::LightTraits Output::get_traits() {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
}

void Output::write_state(light::LightState *state) {
    auto address = libdali::Address::from_short_address(this->short_address);

    if (this->restore_brightness.has_value()) {
        ESP_LOGD(TAG, "'%s' Restore", state->get_object_id().c_str());
        auto call = state->make_call();
        call.set_brightness(this->restore_brightness.value());
        call.set_state(true);
        this->restore_brightness.reset();
        call.perform();
        return;
    }
    bool on;
    float brightness;
    state->current_values_as_brightness(&brightness);
    state->current_values_as_binary(&on);
    auto targetBrightness = uint8_t(254.0f * float(brightness));

    ESP_LOGI(TAG, "'%s' write_state: On: %d Brightness: %f, Dali value: %x", state->get_object_id().c_str(), on, brightness, targetBrightness);
    if (!on) {
        targetBrightness = 0;
    }

    auto err = libdali::DirectArc(this->bus, address, targetBrightness);
    if (err) {
        ESP_LOGE(TAG, "'%s' Direct Arc Control failed: %s", state->get_object_id().c_str(), err.text());
    }
}

}  // namespace lw14
}  // namespace esphome
