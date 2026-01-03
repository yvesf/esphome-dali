#pragma once
#include "dali.h"

namespace libdali {

enum class I2CResult {
    OK,
    ERROR,
};

class I2CInterface {
public:
    virtual I2CResult write_register(uint8_t i2cRegister, uint8_t *data, size_t len) = 0;
    virtual I2CResult read_register(uint8_t i2cRegister, uint8_t *data, size_t len) = 0;
    virtual void delay_microseconds(uint32_t us) = 0;
    virtual uint32_t millis() = 0;
};

class LW14Adapter : public DaliBusInterface {
public:
    LW14Adapter(I2CInterface *t) : transport(t) {};
    ErrorCode DaliCommand(uint8_t address, uint8_t data, uint8_t *reply, size_t replyLength, uint32_t timeoutMs = 150);
protected:
    I2CInterface *transport;
};

} // namespace dali
