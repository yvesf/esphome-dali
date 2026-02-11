#include "linuxi2c.h"
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <linux/i2c-dev.h>
#include <list>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

using namespace libdali;

static int initialise(LW14Adapter *bus);
static int blink(LW14Adapter *bus, std::list<std::string> &args);
static int info(LW14Adapter *bus, std::list<std::string> &args);

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << argv[0] << " /dev/i2c-... OPERATION\n";
    std::cout << "OPERATION can be\n";
    std::cout << "  initialise\n";
    std::cout << "  blink N\n";
    std::cout << "      where N is short address\n";
    std::cout << "  info N\n";
    std::cout << "      where N is short address\n";
    return 1;
  }
  std::list<std::string> args(argv + 1, argv + argc);

  auto transport = ConnectLinuxI2C(args.front().c_str(), LW14_DEFAULT_ADDRESS);
  if (!transport) {
    std::cerr << "failed to initialize I2C transport\n";
    return 1;
  }

  args.pop_front();
  auto *bus = new LW14Adapter(*transport);

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

  delete bus;
}

// Address assignment as found in https://github.com/jorticus/esphome-dali
static int initialise(LW14Adapter *bus) {
  // Turn all lights off for Initialise.
  auto err = Off(bus, Broadcast);
  if (err) {
    std::cerr << "Off: " << err << "\n";
    return 1;
  }

  // Delete all existing short addresses.
  err = DataTransferRegister(bus, DA_MASK);
  if (err) {
    std::cerr << "Load DTR0: " << err << "\n";
    return 1;
  }

  err = StoreDTRAsShortAddress(bus, Broadcast);
  if (err) {
    std::cerr << "StoreDTRAsShortAddressBroadcast: " << err << "\n";
    return 1;
  }

  // Terminate other potentially running initialise.
  err = Terminate(bus);
  if (err) {
    std::cerr << "Terminate: " << err << "\n";
    return 1;
  }

  // Start initialisation, all gear will accept addressing commands for 15min.
  err = Initialise(bus, InitialiseMode::ALL);
  if (err) {
    std::cerr << "Initialise: " << err << "\n";
    return 1;
  }

  // Command gears to chose a random address.
  err = Randomise(bus);
  if (err) {
    std::cerr << "Randomise: " << err << "\n";
    return 1;
  }

  // Give gears 100ms time to find their random address.
  bus->delay_microseconds(100000);

  // Start assigning short addresses from 0 on.
  uint8_t short_address_counter = 0;
  while (true) {
    uint32_t addr = 0x000000;
    // Takes 'addr' for the BRN and starts with bit 2^24 .. 2^0.
    // Sets the bit and runs Compare.
    //   If true, there is equipment with smaller address => unset the bit.
    //   If false (no answer), there is no equipment with smaller address => set
    //   the bit.
    for (uint32_t i = 0; i < 24; i++) {
      uint32_t bit = 1ul << static_cast<uint32_t>(23ul - i);
      uint32_t search_addr = addr | bit;
      std::cout << "\rSearching for addr 0b" << std::bitset<24>{search_addr}
                << "  0x" << std::hex << search_addr << std::flush;

      // True if actual address <= search_address
      err = SearchAddrs(bus, SearchAddr(search_addr));
      if (err) {
        std::cerr << "SearchAddrs: " << err << "\n";
        return 1;
      }

      if (auto compare_result = Compare(bus)) {
        if (*compare_result) {
          addr &= ~bit; // Clear the bit (already clear)
        } else {
          addr |= bit; // Set the bit
        }
      } else if (compare_result.error() == ErrorCode::FRAME_ERROR) {
          // likely means more than one device responded, interpret it as DA_YES.
          addr &= ~bit; // Clear the bit (already clear)
      } else {
        std::cerr << "Compare: " << compare_result.error() << "\n";
        return 1;
      }
    }

    std::cout << "\n";

    // If all bits in BRN-address were set and still no gear is found there
    // there is no device left.
    if (addr == 0xFFFFFF) {
      break;
    }

    // Need to increment by one to get the actual address
    addr++;
    std::cout << "Found address: 0x" << std::hex << addr << "\n";

    // Sanity check: Address should still return true for comparison
    err = SearchAddrs(bus, SearchAddr(addr));
    if (err) {
      std::cerr << "SearchAddrs: " << err << "\n";
      return 1;
    }
    if (auto compare_result = Compare(bus)) {
      if (!*compare_result) {
        std::cerr << "Address not matched in sanity check\n";
        continue;
      }
    } else {
      std::cerr << "Sanity Check Compare: " << compare_result.error() << "\n";
      return 1;
    }

    // Execute withdraw to exclude this device from further COMPARE in the
    // initialisation.
    err = SearchAddrs(bus, SearchAddr(addr));
    if (err) {
      std::cerr << "SearchAddrs: " << err << "\n";
      return 1;
    }
    err = Withdraw(bus);
    if (err) {
      std::cerr << "Withdraw:" << err << "\n";
      return 1;
    }

    // Sanity check: Address should no longer respond to COMPARE.
    err = SearchAddrs(bus, SearchAddr(addr));
    if (err) {
      std::cerr << "SearchAddrs: " << err << "\n";
      return 1;
    }
    if (auto compare_result = Compare(bus)) {
      if (*compare_result) {
        std::cerr << "gear did not withdraw (ignoring, continue searching)\n";
        continue;
      }
    } else {
      std::cerr << compare_result.error() << "\n";
      return 1;
    }

    // Program the short address for the found BRN address.
    err = ProgramShortAddress(bus, short_address_counter);
    if (err) {
      std::cerr << "Program short address: " << err << "\n";
      return 1;
    }
    std::cout << "Programmed short address: " << std::dec
              << static_cast<int>(short_address_counter) << "\n";

    auto gear = Address::from_short_address(short_address_counter);
    if (auto verify_result = VerifyShortAddress(bus, gear)) {
      if (!*verify_result) {
        std::cerr << "Verify Short Address: check failed\n";
        return 1;
      }
    } else {
      std::cerr << "Verify Short Address: " << verify_result.error() << "\n";
      return 1;
    }

    if (auto id_number = MemoryBank0GearIdentificationNumber(bus, gear)) {
      std::cout << "ID from memory bank0: " << std::dec
                << static_cast<uint64_t>(*id_number) << "\n";
    } else {
      std::cerr << "Querying id after programming short address: "
                << id_number.error() << "\n";
      return 1;
    }

    short_address_counter++;
  }

  Terminate(bus);

  return 0;
}

static int blink(LW14Adapter *bus, std::list<std::string> &args) {
  auto short_address = static_cast<uint8_t>(std::stoi(args.front()));
  args.pop_front();
  auto gear = Address::from_short_address(short_address);

  if (auto actual_level = QueryActualLevel(bus, gear)) {
    std::cout << "QueryActualLevel: "
              << static_cast<int>(actual_level.value_or(0)) << "\n";
  } else {
    std::cerr << "QueryActualLevel: " << actual_level.error() << "\n";
    return 1;
  }

  if (auto err = DirectArc(bus, gear, 254)) {
    std::cout << err << "\n";
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (auto actual_level = QueryActualLevel(bus, gear)) {
    std::cout << "QueryActualLevel: " << static_cast<int>(*actual_level)
              << "\n";
  } else {
    std::cerr << "QueryActualLevel: " << actual_level.error() << "\n";
    return 1;
  }

  if (auto err = DirectArc(bus, gear, 0)) {
    std::cout << err << "\n";
    return 1;
  }

  if (auto actual_level = QueryActualLevel(bus, gear)) {
    std::cout << "QueryActualLevel: " << static_cast<int>(*actual_level)
              << "\n";
  } else {
    std::cerr << "QueryActualLevel: " << actual_level.error() << "\n";
    return 1;
  }

  return 0;
}

static int info(LW14Adapter *bus, std::list<std::string> &args) {
  auto short_address = static_cast<uint8_t>(std::stoi(args.front()));
  args.pop_front();

  auto address = libdali::Address::from_short_address(short_address);
  std::cout << "Short Address: " << static_cast<int>(short_address) << "\n";

  if (auto status = QueryStatus(bus, address)) {
    std::cout << "QueryStatus: " << *status << "\n";
  } else {
    std::cerr << "QueryStatus: " << status.error() << "\n";
    return 1;
  }

  if (auto actual_level = QueryActualLevel(bus, address)) {
    std::cout << "QueryActualLevel: " << static_cast<int>(*actual_level)
              << "\n";
  } else {
    std::cerr << "QueryActualLevel: " << actual_level.error() << "\n";
  }

  if (auto id_number = MemoryBank0GearIdentificationNumber(bus, address)) {
    std::cout << "MemoryBank0GearIdentificationNumber: " << std::dec
              << static_cast<uint64_t>(*id_number) << "\n";
  } else {
    std::cerr << "MemoryBank0GearIdentificationNumber: " << id_number.error()
              << "\n";
    return 1;
  }

  return 0;
}
