#pragma once

#include <optional>
#include <esphome.h>
#include "dali.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace dali {

class Output : public light::LightOutput, public Component {
public:
    light::LightTraits get_traits() override;
    void setup_state(light::LightState *state) override;
    void write_state(light::LightState *state) override;
    void set_short_address(uint8_t short_address) {
        this->short_address = short_address;
    }
    void set_bus(libdali::BusInterface *bus) { this->bus = bus; }
private:
    std::optional<float> restore_brightness;
    uint8_t short_address;
    libdali::BusInterface *bus;
};

}  // namespace lw14
}  // namespace esphome
