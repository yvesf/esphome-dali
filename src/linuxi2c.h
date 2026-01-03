#pragma once
#include "lw14.h"

namespace libdali {

class LinuxI2C : public I2CInterface {
public:
    LinuxI2C(int fd, uint8_t address) : fd(fd), address(address) {};
    ~LinuxI2C();
    I2CResult write_register(uint8_t i2cRegister, uint8_t *data, size_t len) override;
    I2CResult read_register(uint8_t i2cRegister, uint8_t *data, size_t len) override;
    void delay_microseconds(uint32_t us) override;
    uint32_t millis() override;
private:
    int fd;
    uint8_t address;
};

std::optional<LinuxI2C*> ConnectLinuxI2C(const char* file, uint8_t address);

} // namespace dali
