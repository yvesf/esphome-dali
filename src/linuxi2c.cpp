#include "linuxi2c.h"
#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <string_view>
#include <vector>

#define DEBUG false

namespace libdali {

void println(std::string_view rem, const std::vector<uint8_t>& container);
void println(std::string_view rem, const std::vector<uint8_t>& container)
{
    std::cout << rem.substr(0, rem.size() - 2) << '[';
    bool first{true};
    for (const int x : container)
        std::cout << (first ? first = false, "0x" : ", 0x") << std::hex << static_cast<int>(x);
    std::cout << "]\n";
}

std::optional<LinuxI2C*> ConnectLinuxI2C(const char *file, uint8_t address) {
    int device;
    unsigned long funcs;

    if ((device = open(file, O_RDWR)) < 0) {
        std::cerr << "Failed to open I2C bus" << std::endl;
        return std::nullopt;
    }

    if (ioctl(device, I2C_FUNCS, &funcs) < 0) {
        std::cerr << "ioctl() I2C_FUNCS failed" << std::endl;
        return std::nullopt;
    }

    return std::optional<LinuxI2C*>(new LinuxI2C(device, address));
}

LinuxI2C::~LinuxI2C() {
    close(this->fd_);
}


I2CResult LinuxI2C::write_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    if (ioctl(this->fd_, I2C_SLAVE, this->address_) >= 0) {
        std::vector<uint8_t> buf(1);
        buf[0] = i2cRegister;
        buf.insert(std::next(buf.begin(), 1), data, data + len);
        if (DEBUG) {
            std::cout << "Write Register len=" << std::dec << len << " size=" << buf.size();
            println(" {}", buf);
        }
        auto ret = write(this->fd_, buf.data(), buf.size());
        if (ret != static_cast<int>(buf.size())) {
            std::cerr << "I2C error writing data: " << ret << " " << __FUNCTION__ << std::endl;
            return I2CResult::ERROR;
        }
        return I2CResult::OK;
    }
    std::cerr << "I2C error selecting device: " << __FUNCTION__ << std::endl;
    return I2CResult::ERROR;
}

I2CResult LinuxI2C::read_register(uint8_t i2cRegister, uint8_t *data, size_t len) {
    if (ioctl(this->fd_, I2C_SLAVE, this->address_) >= 0) {
        auto ret = write(this->fd_, &i2cRegister, 1);
        if (ret != 1) {
            std::cerr << "I2C error selecting read register: " << static_cast<int>(i2cRegister) << std::endl;
            return I2CResult::ERROR;
        }

        usleep(1000);

        ret = read(this->fd_, data, len);
        if (ret != static_cast<int>(len)) {
            std::cerr << "I2C error reading from device: " << __FUNCTION__ << std::endl;
            return I2CResult::ERROR;
        }
        if (DEBUG) {
            std::vector<uint8_t> buf;
            buf.insert(buf.begin(), data, data+len);
            std::cout << "Read Register " << std::hex <<
                " address=0x" << static_cast<int>(this->address_) <<
                " register=0x" <<static_cast<int>(i2cRegister) <<
                " len=" << len;
            println(" buf: {}", buf);
        }

        return I2CResult::OK;
    }
    std::cerr << "I2C error selecting device for setting register: " << __FUNCTION__ << std::endl;
    return I2CResult::ERROR;
}

void LinuxI2C::delay_microseconds(uint32_t us) {
    usleep(us);
}

uint32_t LinuxI2C::millis() {
    auto milliseconds_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
    return static_cast<uint32_t>(milliseconds_since_epoch);
}

} // namespace dali
