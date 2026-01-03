#include "lw14.h"

#define DA_TE (1 / (2 * 1.2)) * 1000 			// ~416.67 us
#define DA_FORWARD_FRAME_TIME 			(38 * DA_TE)
#define DA_BACKWARD_FRAME_TIME 			(22 * DA_TE)
#define DA_SETTLING_TIME_FF_TO_FF 		(22 * DA_TE)
#define DA_SETTLING_TIME_BF_TO_FF 		(22 * DA_TE)
#define DA_SETTLING_TIME_FF_TO_BF_MIN 	(7 * DA_TE)
#define DA_SETTLING_TIME_FF_TO_BF_MAX 	(22 * DA_TE)

namespace libdali {

enum class I2CRegister {
    Status    = 0x00, // read only, 1 byte
    Command   = 0x01, // read/write, 3 bytes
    Config    = 0x02, // write only, 1 byte
    Signature = 0xF0, // read only, 6 bytes
    Address   = 0xFE, // write only, 2 bytes
};

enum class I2CStatusRegisterMask : uint8_t {
    BYTE1       = 0x01,
    BYTE2       = 0x02,
    BYTE3       = 0x03,
    TIMEFRAME   = 0x04,
    VALID       = 0x08,
    FRAME_ERROR = 0x10,
    OVERRUN     = 0x20,
    BUSY        = 0x40, // 0: ready, 1: busy
    BUS_FAULT   = 0x80, // 0: OK     1: bus fault
};

ErrorCode LW14Adapter::DaliCommand(uint8_t address, uint8_t data, uint8_t *reply, size_t replyLength, uint32_t timeoutMs) {
    // ESP_LOGI("dali", "TX(%02x, %02x)", address, data);
    auto start = this->transport->millis();

    uint8_t buf[2];
    // wait for non-busy bus.
    for (auto i = 0;; i++) {
        auto err = this->transport->read_register((uint8_t)I2CRegister::Status, &buf[0], 1);
        if (err != I2CResult::OK) {
            return ErrorCode::ERR_I2C_ERROR;
        }
        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::BUS_FAULT) == (uint8_t)I2CStatusRegisterMask::BUS_FAULT) {
            return ErrorCode::ERR_BUS_ERROR;
        }
        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::VALID) == (uint8_t)I2CStatusRegisterMask::VALID) {
            //ESP_LOGE("DALI", "Clear telegram"); // old telegram stored, clear.
            this->transport->read_register((uint8_t)I2CRegister::Command, &buf[0], 1);
            continue;
        }
        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::BUSY) == 0) {
            break;
        }
        // wait 25 potential frames for non busy bus.
        if (i > 25) {
            return ErrorCode::ERR_TIMEOUT;
        }

        this->transport->delay_microseconds(DA_FORWARD_FRAME_TIME);
    }

    buf[0] = address;
    buf[1] = data;
    auto err = this->transport->write_register((uint8_t)I2CRegister::Command, &buf[0], 2);
    if (err != I2CResult::OK) {
        return ErrorCode::ERR_I2C_ERROR;
    }

    // wait for valid reply or non-busy for no result.
    for (auto retry = 0; true; retry++) {
        err = this->transport->read_register((uint8_t)I2CRegister::Status, &buf[0], 1);
        if (err != I2CResult::OK) {
            return ErrorCode::ERR_I2C_ERROR;
        }
        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::FRAME_ERROR) == (uint8_t)I2CStatusRegisterMask::FRAME_ERROR) {
            // stop if frame error is indicated.
            // return ErrorCode::ERR_FRAME_ERROR;
            // ESP_LOGE("dali", "frame error, wait 5*400us");
            this->transport->delay_microseconds(5*DA_TE);
            continue;
        }
        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::BUS_FAULT) == (uint8_t)I2CStatusRegisterMask::BUS_FAULT) {
            // stop if bus is faulty (no Power, short, etc.)
            return ErrorCode::ERR_BUS_ERROR;
        }
        if (replyLength == 0 && (buf[0] & (uint8_t)I2CStatusRegisterMask::BUSY) == 0) {
            return ErrorCode::OK;
        }

        if ((buf[0] & (uint8_t)I2CStatusRegisterMask::VALID) == (uint8_t)I2CStatusRegisterMask::VALID) {
            // break and continue reading reply if ready.
            break;
        }

        if (this->transport->millis() - start > timeoutMs) {
            return ErrorCode::ERR_TIMEOUT;
        }

        this->transport->delay_microseconds(40*DA_TE);
    }

    // Read reply from command register.
    err = this->transport->read_register((uint8_t)I2CRegister::Command, reply, replyLength);
    if (err != I2CResult::OK) {
        return ErrorCode::ERR_I2C_ERROR;
    }
    // ESP_LOGI("dali", "RX = %02x", reply[0]);
    return ErrorCode::OK;
}
} // namespace dali
