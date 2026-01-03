#include "esphome_bus.h"
#include "esphome/core/log.h"
#include "esphome.h"

namespace esphome {
namespace dali {

static const char *const TAG = "dali";

void Bus::setup() {
}

libdali::I2CResult Bus::write_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    auto err = i2c::I2CDevice::write_register(i2cRegister, data, len);
    if (err) {
        ESP_LOGE(TAG, "Write register %d failed: %d", i2cRegister, err);
        return libdali::I2CResult::ERROR;
    }
    return libdali::I2CResult::OK;
}

libdali::I2CResult Bus::read_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    auto err = i2c::I2CDevice::read_register(i2cRegister, data, len);
    if (err) {
        ESP_LOGE(TAG, "Read register %d failed: %d", i2cRegister, err);
        return libdali::I2CResult::ERROR;
    }
    return libdali::I2CResult::OK;
}


void Bus::dump_config() {
    ESP_LOGCONFIG(TAG, "DaliComponent");
}

}  // namespace lw14
}  // namespace esphome
