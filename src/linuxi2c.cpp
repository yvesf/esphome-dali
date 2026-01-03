#include "linuxi2c.h"
#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

namespace libdali {

std::optional<LinuxI2C*> ConnectLinuxI2C(const char *file, uint8_t address) {
    int device;
    unsigned long funcs;

    if ((device = open("/dev/i2c-17", O_RDWR)) < 0) {
        std::cerr << "Failed to open I2C bus\n" << std::endl;
        return std::nullopt;
    }


    if (ioctl(device, I2C_FUNCS, &funcs) < 0) {
        std::cerr << "ioctl() I2C_FUNCS failed" << std::endl;
        return std::nullopt;
    }

    return std::optional<LinuxI2C*>(new LinuxI2C(device, address));
}

LinuxI2C::~LinuxI2C() {
    close(this->fd);
}

I2CResult LinuxI2C::write_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    int ret = 0;
    if (ioctl(this->fd, I2C_SLAVE, this->address) >= 0) {
        ret = write(this->fd, data, len);
        if (ret != len)
        {
            std::cout << "ERROR: " << __FUNCTION__ << std::endl;
            return I2CResult::ERROR;
        }
        return I2CResult::OK;
    } else {
        std::cout << "ERROR: " << __FUNCTION__ << std::endl;
        return I2CResult::ERROR;
    }
}

I2CResult LinuxI2C::read_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    int ret = 0;
    if (ioctl(this->fd, I2C_SLAVE, this->address) >= 0) {
        ret = read(fd, data, len);
        if (ret != len) {
            std::cout << "ERROR: " << __FUNCTION__ << std::endl;
            return I2CResult::ERROR;
        }
        return I2CResult::OK;
    } else {
        std::cout << "ERROR: " << __FUNCTION__ << std::endl;
        return I2CResult::ERROR;
    }
}

void LinuxI2C::delay_microseconds(uint32_t us) {
    usleep(us);
}

uint32_t LinuxI2C::millis() {
    unsigned long milliseconds_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
    return milliseconds_since_epoch;
}

} // namespace dali
