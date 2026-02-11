#include "lw14.h"
#include <bitset>
#include <cstdint>
#include <sstream>

namespace libdali {

struct I2CRegister {
  struct I2cRegister {
    uint8_t address;
    uint8_t size;
  };
  static constexpr I2cRegister STATUS{.address = 0x00, .size = 1};
  static constexpr I2cRegister COMMAND{.address = 0x01, .size = 1};
  static constexpr I2cRegister CONFIG{.address = 0x02, .size = 1};
  static constexpr I2cRegister SIGNATURE{.address = 0xf0, .size = 1};
  static constexpr I2cRegister ADDRESS{.address = 0xfe, .size = 1};
};

// The status register is one byte that contains the bus status and command
// status flags.
class I2CRegisterStatusValue : std::bitset<I2CRegister::STATUS.size*8> {
public:
  explicit I2CRegisterStatusValue(uint8_t v)
      : std::bitset<I2CRegister::STATUS.size*8>(v) {}
  // LSB byte count for telegram received
  bool lsb_byte_count() const { return this->test(0); }
  // MSB byte count for telegram received
  bool msb_byte_count() const { return this->test(1); }
  // true if less than 22 Te since last command
  bool reply_timeframe() const { return this->test(2); }
  bool valid_reply() const { return this->test(3); }
  bool frame_error() const { return this->test(4); }
  bool overrun() const { return this->test(5); }
  bool busy() const { return this->test(6); }
  bool bus_error() const { return this->test(7); }
  explicit operator std::string() const noexcept {
    std::ostringstream buf;
    buf << "lsb_byte_count: " << this->lsb_byte_count()
        << ",msb_byte_count: " << this->msb_byte_count()
        << ",reply_timeframe: " << this->reply_timeframe()
        << ",valid_reply: " << this->valid_reply()
        << ",frame_error: " << this->frame_error()
        << ",overrun: " << this->overrun() << ",busy: " << this->busy()
        << ",bus_error: " << this->bus_error();
    return buf.str();
  }
};

ErrorCode LW14Adapter::DaliCommand(uint8_t address, uint8_t data,
                                   uint8_t *reply, size_t reply_length,
                                   uint32_t timeout_ms) {
  uint8_t buf[2];
  // wait for non-busy bus.
  for (auto i = 0;; i++) {
    auto err =
        this->transport->read_register(I2CRegister::STATUS.address, &buf[0], 1);
    if (err != I2CResult::OK) {
      return ErrorCode::I2C_ERROR;
    }
    auto status = I2CRegisterStatusValue(buf[0]);
    if (status.bus_error()) {
      return ErrorCode::BUS_ERROR;
    }
    if (status.valid_reply()) {
      // ESP_LOGE("DALI", "Clear telegram"); // old telegram stored, clear.
      this->transport->read_register(I2CRegister::COMMAND.address, &buf[0], 1);
      continue;
    }
    if (!status.busy() && !status.reply_timeframe()) {
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
  auto err =
      this->transport->write_register(I2CRegister::COMMAND.address, &buf[0], 2);
  if (err != I2CResult::OK) {
    return ErrorCode::I2C_ERROR;
  }

  // wait generally 50ms before checking response.
  // Without this on the esp32 I get overlapping responses.
  this->transport->delay_microseconds(50000);

  // wait for valid reply or non-busy for no result.
  auto start = this->transport->millis();
  while (true) {
    err =
        this->transport->read_register(I2CRegister::STATUS.address, &buf[0], 1);
    if (err != I2CResult::OK) {
      return ErrorCode::I2C_ERROR;
    }
    auto status = I2CRegisterStatusValue(buf[0]);
    if (status.frame_error()) {
      // On broadcasts that can mean more than one devices responded.
      return ErrorCode::FRAME_ERROR;
    }
    if (status.bus_error()) { // stop if bus is faulty (no Power, short, etc.)
      return ErrorCode::BUS_ERROR;
    }
    if (status.overrun()) {
        std::cout << "bus error" << std::endl;
    }

    if (!status.busy() && reply_length == 0) {
      return ErrorCode::OK;
    }

    if (status.valid_reply()) {
      // break and continue reading reply if ready.
      break;
    }

    if (this->transport->millis() - start > timeout_ms) {
      return ErrorCode::TIMEOUT;
    }
  }

  // Read reply from command register.
  err = this->transport->read_register(I2CRegister::COMMAND.address, reply,
                                       reply_length);
  if (err != I2CResult::OK) {
    return ErrorCode::I2C_ERROR;
  }

  return ErrorCode::OK;
}
} // namespace libdali
