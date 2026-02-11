#pragma once
#include "dali.h"

namespace libdali {

enum class I2CResult {
  OK,
  ERROR,
};

class I2CInterface {
public:
  virtual ~I2CInterface() = default;
  virtual I2CResult write_register(uint8_t i2cRegister, uint8_t *data,
                                   size_t len) = 0;
  virtual I2CResult read_register(uint8_t i2cRegister, uint8_t *data,
                                  size_t len) = 0;
  virtual void delay_microseconds(uint32_t us) = 0;
  virtual uint32_t millis() = 0;
};

constexpr uint8_t LW14_DEFAULT_ADDRESS = 0x23;

class LW14Adapter : public BusInterface {
public:
  LW14Adapter(I2CInterface *t) : transport(t) {}
  LW14Adapter(const LW14Adapter &o) = delete;
  virtual ErrorCode DaliCommand(uint8_t address, uint8_t data, uint8_t *reply,
                                size_t reply_length,
                                uint32_t timeout_ms) override;
  LW14Adapter &operator=(const LW14Adapter &o) = delete;
  virtual void delay_microseconds(uint32_t us) override {
    this->transport->delay_microseconds(us);
  }

protected:
  I2CInterface *transport;
};

} // namespace libdali
