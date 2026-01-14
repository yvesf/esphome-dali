#include "lw14.h"
#include <bitset>
#include <sstream>

namespace libdali {

enum class I2CRegister {
    Status    = 0x00, // read only, 1 byte
    Command   = 0x01, // read/write, 3 bytes
    Config    = 0x02, // write only, 1 byte
    Signature = 0xF0, // read only, 6 bytes
    Address   = 0xFE, // write only, 2 bytes
};

// The status register is one byte that contains the bus status and command status flags.
class I2CRegisterStatusValue : std::bitset<8> {
public:
    I2CRegisterStatusValue(uint8_t v) : std::bitset<8>(v) {}
    // LSB byte count for telegram received
    bool lsbByteCount() const{ return this->test(0); }
    // MSB byte count for telegram received
    bool msbByteCount() const { return this->test(1); }
    // true if less than 22 Te since last command
    bool replyTimeframe() const { return this->test(2); }
    bool validReply() const { return this->test(3); }
    bool frameError() const { return this->test(4); }
    bool overrun() const { return this->test(5); }
    bool busy() const { return this->test(6); }
    bool busError() const { return this->test(7); }
    explicit operator std::string() const noexcept {
        std::ostringstream buf;
        buf << "lsbByteCount: " << this->lsbByteCount()
            << ",msbByteCount: " << this->msbByteCount()
            << ",replyTimeframe: " << this->replyTimeframe()
            << ",validReply: " << this->validReply()
            << ",frameError: " << this->frameError()
            << ",overrun: " << this->overrun()
            << ",busy: " << this->busy()
            << ",busError: " << this->busError();
        return buf.str();
    }
};

ErrorCode LW14Adapter::DaliCommand(uint8_t address, uint8_t data, uint8_t *reply, size_t replyLength, uint32_t timeoutMs) {
    uint8_t buf[2];
    // wait for non-busy bus.
    for (auto i = 0;; i++) {
        auto err = this->transport->read_register(static_cast<uint8_t>(I2CRegister::Status), &buf[0], 1);
        if (err != I2CResult::OK) {
            return ErrorCode::I2C_ERROR;
        }
        auto status = I2CRegisterStatusValue(buf[0]);
        if (status.busError()) {
            return ErrorCode::BUS_ERROR;
        }
        if (status.validReply()) {
            //ESP_LOGE("DALI", "Clear telegram"); // old telegram stored, clear.
            this->transport->read_register(static_cast<uint8_t>(I2CRegister::Command), &buf[0], 1);
            continue;
        }
        if (!status.busy() && !status.replyTimeframe()) {
            break;
        }
        // wait 25 iterations for non busy bus.
        if (i > 25) {
            return ErrorCode::BUS_BUSY;
        }

        this->transport->delay_microseconds(10000); // wait 10ms
    }

    buf[0] = address;
    buf[1] = data;
    auto err = this->transport->write_register(static_cast<uint8_t>(I2CRegister::Command), &buf[0], 2);
    if (err != I2CResult::OK) {
        return ErrorCode::I2C_ERROR;
    }

    // wait generally 50ms before checking response.
    // Without this on the esp32 I get overlapping responses.
    this->transport->delay_microseconds(50000);

    // wait for valid reply or non-busy for no result.
    auto start = this->transport->millis();
    for (auto retry = 0; true; retry++) {
        err = this->transport->read_register(static_cast<uint8_t>(I2CRegister::Status), &buf[0], 1);
        if (err != I2CResult::OK) {
            return ErrorCode::I2C_ERROR;
        }
        auto status = I2CRegisterStatusValue(buf[0]);
        if (status.frameError()) {
            this->transport->delay_microseconds(10000);
            continue;
        }
        if (status.busError()) { // stop if bus is faulty (no Power, short, etc.)
            return ErrorCode::BUS_ERROR;
        }
        if (status.overrun()) {
            return ErrorCode::BUS_ERROR;
        }

        if (!status.busy() && replyLength == 0) {
            return ErrorCode::OK;
        }

        if (status.validReply()) {
            // break and continue reading reply if ready.
            break;
        }

        if (this->transport->millis() - start > timeoutMs) {
            return ErrorCode::TIMEOUT;
        }

        this->transport->delay_microseconds(10000); // wait 10ms
    }

    // Read reply from command register.
    err = this->transport->read_register(static_cast<uint8_t>(I2CRegister::Command), reply, replyLength);
    if (err != I2CResult::OK) {
        return ErrorCode::I2C_ERROR;
    }

    return ErrorCode::OK;
}
} // namespace dali
