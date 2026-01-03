#pragma once
#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>
#include <iostream>

namespace libdali {

struct ErrorCode {
    enum code_t { OK, ERR_TIMEOUT, ERR_BUS_ERROR, ERR_FRAME_ERROR, ERR_I2C_ERROR};
    constexpr ErrorCode(code_t c = OK) : c(c) {}
    constexpr operator const char* const() const {
        return strings[(char)c];
    }
    friend std::ostream& operator<<(std::ostream& os, const ErrorCode& e) {
        os << strings[(char)e.c];
        return os;
    }
    constexpr operator const bool() const {
        return c != OK;
    }
    constexpr operator code_t() const { return c; }
private:
    code_t c;
    constexpr static const char* strings[5] = {"OK", "Error: timeout", "Error: bus error", "Error: frame error", "Error: i2c error"};
};

template<typename T> class Result {
private:
    ErrorCode error_;
    std::optional<T> value_;
    Result() {};
public:
    Result(ErrorCode error) : error_(error) { }
    Result(T value) : error_(ErrorCode::OK), value_(value) { }
    constexpr explicit operator bool() const noexcept {
        return this->value_.has_value();
    }
    T value() { return this->value_.value(); }
    ErrorCode error() { return this->error_; }
};

class DaliBusInterface {
public:
    virtual ErrorCode DaliCommand(uint8_t address, uint8_t data, uint8_t *reply, size_t replyLength, uint32_t timeoutMs = 150) = 0;
};

constexpr static const auto DA_MODE_COMMAND = 0x01;
constexpr static uint8_t short_address_command(uint8_t address) {
    return ((address & 63) << 1) | DA_MODE_COMMAND;
}

constexpr static const auto DA_MODE_DACP = 0x00;
constexpr uint8_t short_address_dacp(uint8_t address) {
    return ((address & 63) << 1) | DA_MODE_DACP;
}

class Device {
public:
    Device () {}
    Device(uint8_t short_address, DaliBusInterface *bus) {
        this->set_short_address(short_address);
        this->set_bus(bus);
    }
    void set_short_address(uint8_t short_address) { this->short_address = short_address; };
    uint8_t get_short_address() { return this->short_address; }
    void set_bus(DaliBusInterface *bus) { this->bus = bus; }
    DaliBusInterface *get_bus() { return this->bus; }
protected:
    uint8_t short_address;
    DaliBusInterface *bus;
};

static ErrorCode DirectArc(Device *dev, uint8_t power) {
    auto err = dev->get_bus()->DaliCommand(short_address_dacp(dev->get_short_address()),
                                           power,
                                           NULL, 0);
    return err;
}

// Command 257: DATA TRANSFER REGISTER (DTR)
// Stores value in DTR0.
static ErrorCode DataTransferRegister(Device *dev, uint8_t value) {
    return dev->get_bus()->DaliCommand(0xa3, value, nullptr, 0);
}

// Command 273: DATA TRANSFER REGISTER 1 (DTR1)
static ErrorCode DataTransferRegister1(Device *dev, uint8_t value) {
    return dev->get_bus()->DaliCommand(0xc3, value, nullptr, 0);
}

struct ControlCommand {
    uint8_t command;
    ErrorCode operator ()(Device *dev) const {
        return dev->get_bus()->DaliCommand(short_address_command(dev->get_short_address()),
                                               this->command,
                                               nullptr, 0);
    }
};

// Command 0: OFF
constexpr static const ControlCommand Off{0x00};
// Command 1: UP
constexpr static const ControlCommand Up{0x01};
// Command 2: DOWN
constexpr static const ControlCommand Down{0x02};
// Command 3: STEP UP
constexpr static const ControlCommand StepUp{0x03};
// Command 4: STEP DOWN
constexpr static const ControlCommand StepDown{0x04};
// Command 5: RECALL MAX LEVEL
constexpr static const ControlCommand RecallMaxLevel{0x05};
// Command 6: RECALL MIN LEVEL
constexpr static const ControlCommand RecallMinLevel{0x06};
// Command 7: STEP DOWN AND OFF
constexpr static const ControlCommand StepDownAndOff{0x07};
// Command 8: ON AND STEP UP
constexpr static const ControlCommand OnAndStepUp{0x08};
// Command 9: ENABLE DAPC SEQUENCE
constexpr static const ControlCommand EnableDAPCSequence{0x09};


template <typename T> struct QueryCommand {
    uint8_t command;
    Result<T> operator ()(Device *dev) const {
        uint8_t buf[1];
        auto err = dev->get_bus()->DaliCommand(short_address_command(dev->get_short_address()),
                                               this->command,
                                               &buf[0], 1);
        if (err) {
            return Result<T>(err);
        }
        T value = buf[0];
        return Result<T>(value);
    }
};

// Command 144: QUERY STATUS
struct QueryStatusResponse {
    bool StatusOK;
    bool LampFailure;
    bool LampArcPowerOn ;
    bool LimitError;
    bool FadeReady ;
    bool QueryResetState;
    bool QueryMissingShortAddress;
    bool QueryPowerFailure;
    QueryStatusResponse &operator=(const uint8_t& result) {
        this->StatusOK = (result>>0) & 0x01;
        this->LampFailure = (result >> 1) & 0x01;
        this->LampArcPowerOn = (result >> 2) & 0x01;
        this->LimitError = (result >> 3) & 0x01;
        this->FadeReady = (result >> 4) & 0x01;
        this->QueryResetState = (result >> 5) & 0x01;
        this->QueryMissingShortAddress = (result >> 6) & 0x01;
        this->QueryPowerFailure = (result >> 7) & 0x01;
        return *this;
    }
};
constexpr static const QueryCommand<QueryStatusResponse> QueryStatus{0x90};

// Command 160: QUERY ACTUAL LEVEL
constexpr static const QueryCommand<uint8_t> QueryActualLevel{0xa0};

struct DTR0Command {
    uint8_t command;
    ErrorCode operator ()(Device *dev, uint8_t dtr0) const {
        auto err = DataTransferRegister(dev, dtr0);
        if (err) {
            return err;
        }

        err = dev->get_bus()->DaliCommand(short_address_command(dev->get_short_address()), this->command, nullptr, 0);
        return err;
    }
};

// Command 227: SELECT DIMMING CURVE
// DTR = 1: linear curve. DTR = 0: logarithmic curve.
constexpr static const DTR0Command SelectDimmingCurve{0xE3};

// Access to memory banks
template <typename T> struct MemoryUInt64 {
  using Buf = T;
  MemoryUInt64 &operator=(const Buf& result) {
      this->value = 0;
      for (auto i = 0; i<sizeof(Buf); i++) {
          this->value  = this->value  + ( ((int64_t)result[i]) << ((sizeof(Buf)-i)*8) );
      }
      return *this;
  }
  constexpr explicit operator uint64_t() const noexcept {
      return this->value;
  }
private:
    uint64_t value;
};


template <typename T> struct ReadMemory {
    uint8_t bank;
    uint8_t location;
    using Buf_t = typename T::Buf;
    Result<T> operator ()(Device *dev) const {
        auto err = DataTransferRegister1(dev, bank);
        if (err) {
            return err;
        }

        err = DataTransferRegister(dev, location);
        if (err) {
            return err;
        }

        constexpr static auto DA_READ_MEMORY_LOCATION = 0xC5;

        uint8_t buf[sizeof(Buf_t)];
        for (auto i = 0; i<sizeof(Buf_t); i++) {
            err = dev->get_bus()->DaliCommand(short_address_command(dev->get_short_address()),
                                                   DA_READ_MEMORY_LOCATION,
                                                   &buf[i], 1);
            if (err) {
                return Result<T>(err);
            }
        }
        T value = buf[0];
        return Result<T>(value);
    }
};

// Global Trade Item Number
constexpr static const ReadMemory<MemoryUInt64<char[6]>> MemoryBank0GTIN{0, 0x03};
// Identification or serial number of the bus unit
constexpr static const ReadMemory<MemoryUInt64<char[8]>> MemoryBank0GearIdentificationNumber{0, 0x0b};

} // namespace libdali
