#include <iostream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <fcntl.h>  // Für open() und O_RDWR
#include <unistd.h> // Für close()
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "linuxi2c.h"

using namespace libdali;

int main() {
    auto transport = ConnectLinuxI2C("/dev/i2c-17", 0x23);
    if (!transport) {
        return 1;
    }

    auto bus = new LW14Adapter(*transport);
    auto device = new Device();

    auto err = DirectArc(device, 254);
    if (err) {
        std::cout << err << std::endl;
        return 1;
    }
    std::this_thread::sleep_for (std::chrono::seconds(1));

    err = DirectArc(device, 0);
    if (err) {
        std::cout << err << std::endl;
        return 1;
    }

    return 0;
}
