#pragma once
#include <unistd.h>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

namespace libdali {

struct ErrorCode {
    enum code_t { OK, TIMEOUT, BUS_BUSY, BUS_ERROR, FRAME_ERROR, I2C_ERROR};
    constexpr ErrorCode(code_t c = OK) : c_(c) {}
    constexpr const char* text() const {
        return strings[static_cast<int>(c_)];
    }
    constexpr operator const char*() const {
        return text();
    }
    friend std::ostream& operator<<(std::ostream& os, const ErrorCode& e) {
        os << strings[static_cast<int>(e.c_)];
        return os;
    }
    constexpr operator bool() {
        return c_ != OK;
    }
    constexpr operator code_t() const { return c_; }
private:
    code_t c_;
    constexpr static const char* strings[6] = {"OK", "Error: timeout", "Error: Bus busy", "Error: bus error", "Error: frame error", "Error: i2c error"};
};

template<typename T> class Result : public std::optional<T> {
private:
    ErrorCode error_;
public:
    Result(T value) : std::optional<T>(value), error_(ErrorCode::OK) { }
    Result(ErrorCode error) : std::optional<T>(), error_(error) { }
    ErrorCode error() { return this->error_; }
};

class BusInterface {
public:
    virtual ~BusInterface() {}
    virtual ErrorCode DaliCommand(uint8_t address, uint8_t data, uint8_t *reply, size_t replyLength, uint32_t timeoutMs = 150) = 0;
};

constexpr static const uint8_t DA_MASK = 0xff;

class Address {
    constexpr static const uint8_t DA_MODE_COMMAND = 0x01;
    constexpr static const uint8_t DA_MODE_DACP = 0x00;
    uint8_t address_;

    public:
    Address(uint8_t address) : address_(address) {}
    uint8_t command() {
        return (this->address_ << 1) | DA_MODE_COMMAND;
    }
    uint8_t dacp() {
        return (this->address_ << 1) | DA_MODE_DACP;
    }
    static Address from_short_address(uint8_t shortAddress) {
        return Address(shortAddress & 63);
    }
};

static Address Broadcast(0xfe);

static ErrorCode DataTransferRegister(BusInterface *bus, uint8_t value);

// DirectArc command.
static ErrorCode DirectArc(BusInterface *bus, Address address, uint8_t power) {
    auto err = bus->DaliCommand(address.dacp(), power, NULL, 0);
    return err;
}


struct ControlCommand {
    uint8_t command;
    ErrorCode operator ()(BusInterface *bus, Address address) const {
        return bus->DaliCommand(address.command(), this->command, nullptr, 0);
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

// Command 128: STORE DTR AS SHORT ADDRESS
// This command will be send twice.
static ErrorCode StoreDTRAsShortAddress(BusInterface *bus, Address address) {
    auto err = bus->DaliCommand(address.command(), 0x80, nullptr, 0);
    if (err) {
        return err;
    }
    return bus->DaliCommand(address.command(), 0x80, nullptr, 0);
}

template <typename T> struct QueryCommand {
    uint8_t command;
    Result<T> operator ()(BusInterface *bus, Address address) const {
        uint8_t buf[1];
        auto err = bus->DaliCommand(address.command(), this->command, &buf[0], 1);
        if (err) {
            return Result<T>(err);
        }
        T value(buf[0]);
        return Result<T>(value);
    }
};

// Command 144: QUERY STATUS
struct QueryStatusResponse {
    bool StatusOK, LampFailure, LampArcPowerOn, LimitError, FadeReady, QueryResetState, QueryMissingShortAddress, QueryPowerFailure;
    QueryStatusResponse(const uint8_t& result) :
        StatusOK((result>>0) & 0x01),
        LampFailure((result >> 1) & 0x01),
        LampArcPowerOn((result >> 2) & 0x01),
        LimitError((result >> 3) & 0x01),
        FadeReady((result >> 4) & 0x01),
        QueryResetState((result >> 5) & 0x01),
        QueryMissingShortAddress((result >> 6) & 0x01),
        QueryPowerFailure((result >> 7) & 0x01) {}
    friend std::ostream& operator<<(std::ostream& os, const QueryStatusResponse& e) {
        os << "StatusOK=" << e.StatusOK << ",LampFailure=" << e.LampFailure
           << ",LampArcPowerOn=" << e.LampArcPowerOn << ",LimitError=" << e.LimitError
           << ",FadeReady=" << e.FadeReady << ",QueryResetState=" << e.QueryResetState
           << ",QueryMissingShortAddress=" << e.QueryMissingShortAddress << ",QueryPowerFailure=" << e.QueryPowerFailure;
        return os;
    }
};
constexpr static const QueryCommand<QueryStatusResponse> QueryStatus{0x90};

// Command 160: QUERY ACTUAL LEVEL
constexpr static const QueryCommand<uint8_t> QueryActualLevel{0xa0};


struct DTR0Command {
    uint8_t command;
    ErrorCode operator ()(BusInterface *bus, Address address, uint8_t dtr0) const {
        auto err = DataTransferRegister(bus, dtr0);
        if (err) {
            return err;
        }

        err = bus->DaliCommand(address.command(), this->command, nullptr, 0);
        return err;
    }
};

// Command 227: SELECT DIMMING CURVE
// DTR = 1: linear curve. DTR = 0: logarithmic curve.
constexpr static const DTR0Command SelectDimmingCurve{0xE3};

// Command 237: QUERY GEAR TYPE
struct QueryGearTypeResponse {
    bool ledSupplyIntegrated, ledModuleIntegrated, acSupply, dcSupply;
    QueryGearTypeResponse(const uint8_t& result) :
        ledSupplyIntegrated((result >> 0) & 0x01),
        ledModuleIntegrated((result >> 1) & 0x01),
        acSupply((result >> 2) & 0x01),
        dcSupply((result >> 3) & 0x01) {}
    friend std::ostream& operator<<(std::ostream& os, const QueryGearTypeResponse& e) {
        os << "ledSupplyIntegrated=" << e.ledSupplyIntegrated << ",ledModuleIntegrated" << e.ledModuleIntegrated
           << ",acSupply=" << e.acSupply << ",dcSupply=" << e.dcSupply;
        return os;
    }
};
constexpr static const QueryCommand<QueryGearTypeResponse> QueryGearType{0xed};

// Command 238: QUERY DIMMING CURVE
struct QueryDimmingCurveResponse {
    uint8_t value;
    QueryDimmingCurveResponse(const uint8_t result) : value(result) {};
    constexpr bool logarithmic() const { return this->value == 0; }
    constexpr bool linear() const { return this->value == 1; }
    friend std::ostream& operator<<(std::ostream& os, const QueryDimmingCurveResponse& e) {
        os << (e.linear() ? "linear" : "logarithmic");
        return os;
    }
};
constexpr static const QueryCommand<QueryDimmingCurveResponse> QueryDimmingCurve{0xee};

// Command 239: QUERY POSSIBLE OPERATING MODES
struct QueryPossibleOperatingModesResponse : std::bitset<8> {
    QueryPossibleOperatingModesResponse(uint8_t v) : std::bitset<8>(v) {}
    bool pwmPossible() const { return this->test(0); }
    bool amPossible() const { return this->test(1); }
    bool outputCurrentRegulateable() const { return this->test(2); }
    bool highCurrentPulsePossible() const { return this->test(3); }
    friend std::ostream& operator<<(std::ostream& os, const QueryPossibleOperatingModesResponse& e) {
        os << "pwmPossible=" << e.pwmPossible() << ",amPossible" << e.amPossible()
           << ",outputCurrentRegulateable=" << e.outputCurrentRegulateable() << ",highCurrentPulsePossible=" << e.highCurrentPulsePossible();
        return os;
    }
};
constexpr static const QueryCommand<QueryPossibleOperatingModesResponse> QueryPossibleOperatingModes{0xef};

// Command 240: QUERY FEATURES
constexpr static const QueryCommand<uint8_t> QueryFeatures{0xf0};

// Command 241: QUERY FAILURE STATUS
constexpr static const QueryCommand<uint8_t> QueryFailureStatus{0xf1};

// Command 242: QUERY SHORT CIRCUIT
constexpr static const QueryCommand<bool> QueryShortCircuit{0xf2};

// Command 250: COMPARE
// A gear will respond with "yes" (0xff) => true, if it's
// BRN is smaller or equal to the current SEARCHADDR.
static Result<bool> Compare(BusInterface *bus) {
    uint8_t buf[1];
    auto err = bus->DaliCommand(0xa9, 0, &buf[0], 1);
    if (err == ErrorCode::TIMEOUT) {
        return Result<bool>(false);
    }
    if (err) {
        return Result<bool>(err);
    }
    return Result<bool>(buf[0] == 0xff);
}

// Command 251: TERMINATE
static ErrorCode Terminate(BusInterface *bus) {
    return bus->DaliCommand(0xa1, 0x00, nullptr, 0);
}

// Command 252: QUERY OPERATING MODE
struct QueryOperatingModeResponse : std::bitset<8> {
    QueryOperatingModeResponse(uint8_t v) : std::bitset<8>(v) {}
    bool pwmActive() const { return this->test(0); }
    bool amActive() const { return this->test(1); }
    bool outputCurrentRegulated() const { return this->test(2); }
    bool highCurrentPulseActive() const { return this->test(3); }
    bool nonLogarithmicDimmingActive() const { return this->test(4); }
    friend std::ostream& operator<<(std::ostream& os, const QueryOperatingModeResponse& e) {
        os << "pwmActive=" << e.pwmActive() << ",amActive" << e.amActive()
           << ",outputCurrentRegulated=" << e.outputCurrentRegulated()
           << ",highCurrentPulseActive=" << e.highCurrentPulseActive()
           << ",nonLogarithmicDimmingActive=" << e.nonLogarithmicDimmingActive();
        return os;
    }
};
constexpr static const QueryCommand<QueryOperatingModeResponse> QueryOperatingMode{0xfc};

// Command 261: WITHDRAW
static ErrorCode Withdraw(BusInterface *bus) {
    return bus->DaliCommand(0xab, 0x00, nullptr, 0);
}

// Command 257: DATA TRANSFER REGISTER (DTR)
// Stores value in DTR0.
static ErrorCode DataTransferRegister(BusInterface *bus, uint8_t value) {
    return bus->DaliCommand(0xa3, value, nullptr, 0);
}

enum class InitialiseMode {
    ALL = 0x00, // All control gear shall react
    NEW = 0xff, // Control gear without short address shall react
};

// Command 258: INITIALISE
static ErrorCode Initialise(BusInterface *bus, InitialiseMode mode) {
    auto err = bus->DaliCommand(0xa5, static_cast<uint8_t>(mode), nullptr, 0);
    if (err) {
        return err;
    }
    return bus->DaliCommand(0xa5, static_cast<uint8_t>(mode), nullptr, 0);
}

// Command 258: INITIALISE with address.
static ErrorCode Initialise(BusInterface *bus, Address address) {
    auto err = bus->DaliCommand(0xa5, address.command(), nullptr, 0);
    if (err) {
        return err;
    }
    usleep(1000);
    return bus->DaliCommand(0xa5, address.command(), nullptr, 0);
}

// Command 259: RANDOMISE
// Standard defines a gear may up to 100ms to define a new address.
// This function implements sending the command twice.
static ErrorCode Randomise(BusInterface *bus) {
    auto err = bus->DaliCommand(0xa7, 0, nullptr, 0);
    if (err) {
        return err;
    }
    usleep(1000);

    return bus->DaliCommand(0xa7, 0, nullptr, 0);
}

class SearchAddr {
    uint8_t h_, m_, l_;
public:
    SearchAddr(const uint32_t& value) :
        h_((value >> 16) & 0xff),
        m_((value >> 8) & 0xff),
        l_((value >> 0) & 0xff) {}
    SearchAddr(uint8_t h, uint8_t m, uint8_t l) : h_(h), m_(m), l_(l) {}
    uint8_t h() { return this->h_; }
    uint8_t m() { return this->m_; }
    uint8_t l() { return this->l_; }
    constexpr explicit operator int32_t() const noexcept {
        int32_t v = this->h_;
        v <<= 8;
        v += this->m_;
        v <<= 8;
        v += this->l_;
        return v;
    }
};

static const SearchAddr SearchAddrMax = SearchAddr(0x00ffffff);

// Command 264-266: Sets the 24bit search addr.
static ErrorCode SearchAddrs(BusInterface *bus, SearchAddr address) {
    static const uint8_t SEARCHADDRH = 0xb1, SEARCHADDRM = 0xb3, SEARCHADDRL = 0xb5;
    auto err = bus->DaliCommand(SEARCHADDRH, address.h(), nullptr, 0);
    if (err) {
        return err;
    }
    err = bus->DaliCommand(SEARCHADDRM, address.m(), nullptr, 0);
    if (err) {
        return err;
    }
    err = bus->DaliCommand(SEARCHADDRL, address.l(), nullptr, 0);
    return err;
}

// Command 267: PROGRAM SHORT ADDRESS - Set short address.
static ErrorCode ProgramShortAddress(BusInterface *bus, uint8_t shortAddress) {
    return bus->DaliCommand(0xb7, Address::from_short_address(shortAddress & 63).command(), nullptr, 0);
}

// Command 267: PROGRAM SHORT ADDRESS - Delete short address.
static ErrorCode ProgramShortAddressDelete(BusInterface *bus) {
    return bus->DaliCommand(0xb7, 0xff, nullptr, 0);
}

// Command 268: VERIFY SHORT ADDRESS
static Result<bool> VerifyShortAddress(BusInterface *bus, Address address) {
    uint8_t buf[1];
    auto err = bus->DaliCommand(0xb9, address.command(), &buf[0], 1);
    if (err == ErrorCode::TIMEOUT) {
        return Result<bool>(false);
    }
    if (err) {
        return Result<bool>(err);
    }
    return Result<bool>(buf[0] == 0xff);
}

// Command 273: DATA TRANSFER REGISTER 1 (DTR1)
static ErrorCode DataTransferRegister1(BusInterface *bus, uint8_t value) {
    return bus->DaliCommand(0xc3, value, nullptr, 0);
}

// Access to memory banks
template <size_t N> struct MemoryUInt64 {
    static constexpr size_t Size = N;
    MemoryUInt64(const uint8_t *buf) : value(0) {
        for (size_t i = 0; i<Size; i++) {
            this->value = this->value  + ( (static_cast<uint64_t>(buf[i])) << ((Size-i-1)*8) );
        }
    }
    constexpr explicit operator uint64_t() const noexcept { return this->value; }
private:
    uint64_t value;
};

template <typename T> struct ReadMemory {
    uint8_t bank;
    uint8_t location;
    Result<T> operator ()(BusInterface *bus, Address address) const {
        constexpr static auto DA_READ_MEMORY_LOCATION = 0xC5;

        auto err = DataTransferRegister1(bus, bank);
        if (err) {
            return err;
        }

        err = DataTransferRegister(bus, location);
        if (err) {
            return err;
        }

        uint8_t buf[T::Size];
        for (size_t i = 0; i<T::Size; i++) {
            err = bus->DaliCommand(address.command(), DA_READ_MEMORY_LOCATION, &buf[i], 1);
            if (err) {
                return Result<T>(err);
            }
        }
        T value(&buf[0]);
        return Result<T>(value);
    }
};

// Bank 0: Global Trade Item Number
constexpr static const ReadMemory<MemoryUInt64<6>> MemoryBank0GTIN{0, 0x03};
// Bank 0: Identification or serial number of the bus unit
constexpr static const ReadMemory<MemoryUInt64<8>> MemoryBank0GearIdentificationNumber{0, 0x0b};

} // namespace libdali
