#pragma once

#include "lw14.h"

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace dali {

class Bus : public Component,
            public i2c::I2CDevice,
            public libdali::I2CInterface,
            public libdali::LW14Adapter {
public:
  Bus() : libdali::LW14Adapter(this) {};
  // Implement Component.
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }
  // Implement I2CInterface.
  void delay_microseconds(uint32_t us) override { delayMicroseconds(us); };
  uint32_t millis() override { return esphome::millis(); }
  libdali::I2CResult write_register(uint8_t i2c_register, uint8_t *data,
                                    size_t len) override;
  libdali::I2CResult read_register(uint8_t i2c_register, uint8_t *data,
                                   size_t len) override;
};

} // namespace dali
} // namespace esphome
