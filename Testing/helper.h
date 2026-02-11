#pragma once
#include "dali.h"

class Testbus : public libdali::BusInterface {
public:
  libdali::ErrorCode next_error_code = libdali::ErrorCode::OK;
  uint8_t *next_reply = nullptr;
  size_t next_reply_length = 0;
  uint32_t last_delay;
  uint8_t last_address;
  uint8_t last_data;
  size_t last_reply_length;
  uint32_t last_timeout_ms;
  virtual libdali::ErrorCode DaliCommand(uint8_t address, uint8_t data,
                                         uint8_t *reply, size_t reply_length,
                                         uint32_t timeout_ms) override {
    this->last_address = address;
    this->last_data = data;
    for (size_t i = 0; i<reply_length && i<this->next_reply_length; i++) {
        *(reply + i) = *(this->next_reply + i);
    }
    this->last_reply_length = reply_length;
    this->last_timeout_ms = timeout_ms;
    return this->next_error_code;
  }
  virtual void delay_microseconds(uint32_t delay) override {
    this->last_delay = delay;
  };
};
