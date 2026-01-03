#pragma once

#include <optional>
#include <esphome.h>
#include "dali.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace dali {

class Output : public light::LightOutput, public Component, public libdali::Device {
public:
    light::LightTraits get_traits() override;
    void setup_state(light::LightState *state) override;
    void write_state(light::LightState *state) override;
private:
    std::optional<float> restore_brightness;
};

}  // namespace lw14
}  // namespace esphome
