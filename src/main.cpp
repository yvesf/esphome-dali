#include <iostream>
#include <chrono>
#include <thread>
#include <list>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "linuxi2c.h"

using namespace libdali;

int initialise(LW14Adapter *bus);
int blink(LW14Adapter *bus, std::list<std::string>& args);
int info(LW14Adapter *bus, std::list<std::string>& args);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << argv[0] << " /dev/i2c-... OPERATION" << std::endl;
        std::cout << "OPERATION can be" << std::endl;
        std::cout << "  initialise" << std::endl;
        std::cout << "  blink N" << std::endl;
        std::cout << "      where N is short address" << std::endl;
        return 1;
    }
    std::list<std::string> args(argv + 1, argv + argc);

    auto transport = ConnectLinuxI2C(args.front().c_str(), LW14DefaultAddress);
    if (!transport) {
        std::cerr << "failed to initialize I2C transport" << std::endl;
        return 1;
    }

    args.pop_front();
    auto bus = new LW14Adapter(*transport);

    auto op = args.front();
    args.pop_front();
    if (op == "initialise") {
        return initialise(bus);
    } else if (op == "blink") {
        return blink(bus, args);
    } else if (op == "info") {
        return info(bus, args);
    } else if (op == "off") {
        Off(bus, Broadcast);
    }
}

// Address assignment as found in https://github.com/jorticus/esphome-dali
int initialise(LW14Adapter *bus) {
    // Turn all lights off for Initialise.
    auto err = Off(bus, Broadcast);
    if (err) {
        std::cerr << "Off: " << err << std::endl;
        return 1;
    }

    // Delete all existing short addresses.
    err = DataTransferRegister(bus, DA_MASK);
    if (err) {
        std::cerr << "Load DTR0: " << err << std::endl;
        return 1;
    }

    err = StoreDTRAsShortAddress(bus, Broadcast);
    if (err) {
        std::cerr << "StoreDTRAsShortAddressBroadcast: " << err << std::endl;
        return 1;
    }

    // Terminate other potentially running initialise.
    err = Terminate(bus);
    if (err) {
        std::cerr << "Terminate: " << err << std::endl;
        return 1;
    }

    // Start initialisation, this makes all gears accept addressing commands for 15min.
    err = Initialise(bus, InitialiseMode::ALL);
    if (err) {
        std::cerr << "Initialise: " << err << std::endl;
        return 1;
    }

    // Command gears to chose a random address.
    err = Randomise(bus);
    if (err) {
        std::cerr << "Randomise: " << err << std::endl;
        return 1;
    }

    // Give gears 100ms time to find their random address.
    usleep(100000);

    // Start assigning short addresses from 0 on.
    uint8_t shortAddressCounter = 0;
    while (true) {
        uint32_t addr = 0x000000;
        // Takes 'addr' for the BRN and starts with bit 2^24 .. 2^0.
        // Sets the bit and runs Compare.
        //   If true, there is equipment with smaller address => unset the bit.
        //   If false (no answer), there is no equipment with smaller address => set the bit.
        for (uint32_t i = 0; i < 24; i++) {
            uint32_t bit = 1ul << static_cast<uint32_t>(23ul - i);
            uint32_t search_addr = addr | bit;
            std::cout << "Searching for addr 0b" <<
                std::bitset<24>{search_addr} <<
                "  0x" << std::hex << search_addr << " - ";

            // True if actual address <= search_address
            err = SearchAddrs(bus, SearchAddr(search_addr));
            if (err) {
                std::cerr << "SearchAddrs: " << err << std::endl;
                return 1;
            }

            auto compareResult = Compare(bus);
            if (compareResult.error()) {
                std::cerr << "Compare: " << compareResult.error() << std::endl;
                return 1;
            }

            if (compareResult.value()) {
                addr &= ~bit; // Clear the bit (already clear)
                std::cout << "DA_YES - clear the bit" << std::endl;
            } else {
                addr |= bit;  // Set the bit
                std::cout << "TIMEOUT - set the bit" << std::endl;
            }
        }

        // If all bits in BRN-address were set and still no gear is found there there is no device left.
        if (addr == 0xFFFFFF) {
            break;
        }

        // Need to increment by one to get the actual address
        addr++;
        std::cout << "Found address: 0x" << std::hex << addr << std::endl;

        // Sanity check: Address should still return true for comparison
        err = SearchAddrs(bus, SearchAddr(addr));
        if (err) {
            std::cerr << "SearchAddrs: " << err << std::endl;
            return 1;
        }
        auto compareResult = Compare(bus);
        if (compareResult.error()) {
            std::cerr << "Sanity Check Compare: " << compareResult.error() << std::endl;
            return 1;
        }
        if (!compareResult.value()) {
            std::cerr << "Address not matched in sanity check" << std::endl;
            continue;
        }

        // Execute withdraw to exclude this device from further COMPARE in the initialisation.
        err = SearchAddrs(bus, SearchAddr(addr));
        if (err) {
            std::cerr << "SearchAddrs: " << err << std::endl;
            return 1;
        }
        err = Withdraw(bus);
        if (err) {
            std::cerr << "Withdraw:" << err << std::endl;
            return 1;
        }

        // Sanity check: Address should no longer respond to COMPARE.
        err = SearchAddrs(bus, SearchAddr(addr));
        if (err) {
            std::cerr << "SearchAddrs: " << err << std::endl;
            return 1;
        }
        compareResult = Compare(bus);
        if (compareResult.error()) {
            std::cerr << compareResult.error() << std::endl;
            return 1;
        }
        if (compareResult.value() == true) {
            std::cerr << "gear did not withdraw (ignoring, continue searching)" << std::endl;
            continue;
        }

        // Program the short address for the found BRN address.
        err = ProgramShortAddress(bus, shortAddressCounter);
        if (err) {
            std::cerr << "Program short address: " << compareResult.error() << std::endl;
            return 1;
        }
        std::cout << "Programmed short address: " << std::dec << static_cast<int>(shortAddressCounter) << std::endl;

        auto gear = Address::from_short_address(shortAddressCounter);
        auto verifyResult = VerifyShortAddress(bus, gear);
        if (verifyResult.error()) {
            std::cerr << "Verify Short Address: " << verifyResult.error() << std::endl;
            return 1;
        }
        if (!verifyResult.value()) {
            std::cerr << "Verify Short Address: check failed" << std::endl;
            return 1;
        }

        auto id = MemoryBank0GearIdentificationNumber(bus, gear);
        if (id.error()) {
            std::cerr << "Querying id after programming short address: " << id.error() << std::endl;
            return 1;
        }
        std::cout << "ID from memory bank0: " << std::dec << static_cast<uint64_t>(id.value());

        shortAddressCounter++;
    }

    Terminate(bus);

    return 0;
}

int blink(LW14Adapter *bus, std::list<std::string>& args) {
    auto shortAddress = static_cast<uint8_t>(std::stoi(args.front()));
    args.pop_front();
    auto gear = Address::from_short_address(shortAddress);

    auto actualLevel = QueryActualLevel(bus, gear);
    if (actualLevel.error()) {
        std::cerr << "QueryActualLevel: " << actualLevel.error() << std::endl;
        return 1;
    }
    std::cout << "QueryActualLevel: " << static_cast<int>(actualLevel.value()) << std::endl;

    auto err = DirectArc(bus, gear, 254);
    if (err) {
        std::cout << err << std::endl;
        return 1;
    }
    std::this_thread::sleep_for (std::chrono::seconds(1));

    actualLevel = QueryActualLevel(bus, gear);
    if (actualLevel.error()) {
        std::cerr << "QueryActualLevel: " << actualLevel.error() << std::endl;
        return 1;
    }
    std::cout << "QueryActualLevel: " << static_cast<int>(actualLevel.value()) << std::endl;

    err = DirectArc(bus, gear, 0);
    if (err) {
        std::cout << err << std::endl;
        return 1;
    }

    actualLevel = QueryActualLevel(bus, gear);
    if (actualLevel.error()) {
        std::cerr << "QueryActualLevel: " << actualLevel.error() << std::endl;
        return 1;
    }
    std::cout << "QueryActualLevel: " << static_cast<int>(actualLevel.value()) << std::endl;

    return 0;
}

int info(LW14Adapter *bus, std::list<std::string>& args) {
    for (auto x = 0; x<100; x++) {
    for (uint8_t i =0 ; i<6; i++) {
        auto address = libdali::Address::from_short_address(i);
        std::cout << std::dec << static_cast<int>(i) << " ";
        auto status = QueryActualLevel(bus, address);
        if (status.error()) {
            std::cerr << "QueryActualLevel: " << status.error() << std::endl;
            return 1;
        }
        std::cout << "QueryActualLevel: " << static_cast<int>(status.value()) << std::endl;
        switch (i) {
            case 2:
                if (status.value() != 255) {
                    return 1;
                }
                break;
            default:
                if (status.value() != 0) {
                    return 1;
                }
        }
    }
    }
    return 0;

    auto shortAddress = static_cast<uint8_t>(std::stoi(args.front()));
    args.pop_front();

    auto address = libdali::Address::from_short_address(shortAddress);
    std::cout << "Short Address: " << static_cast<int>(shortAddress) << std::endl;

    auto status = QueryStatus(bus, address);
    if (status.error()) {
        std::cerr << "QueryStatus: " << status.error() << std::endl;
        return 1;
    }
    std::cout << "QueryStatus: " << status.value() << std::endl;

    auto actualLevel = QueryActualLevel(bus, address);
    if (actualLevel.error()) {
        std::cerr << "QueryActualLevel: " << actualLevel.error() << std::endl;
    }
    std::cout << "QueryActualLevel: " << static_cast<int>(actualLevel.value()) << std::endl;

    auto idNumber = MemoryBank0GearIdentificationNumber(bus, address);
    if (idNumber.error()) {
        std::cerr << "MemoryBank0GearIdentificationNumber: " << idNumber.error() << std::endl;
        return 1;
    }
    std::cout << "MemoryBank0GearIdentificationNumber: " << std::dec << static_cast<uint64_t>(idNumber.value()) << std::endl;

    return 0;
}
