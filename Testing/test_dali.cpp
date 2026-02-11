#include <catch2/catch_test_macros.hpp>
#include "helper.h"

TEST_CASE("- Direct Arc Control") {
    Testbus bus;
    const auto address = libdali::Address::from_short_address(10);

    SECTION("success 0") {
        libdali::DirectArc(&bus, address, 0);
        REQUIRE(static_cast<int>(bus.last_address) == 10 << 1);
        REQUIRE(static_cast<int>(bus.last_data) == 0x00);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }
    SECTION("success 255") {
        libdali::DirectArc(&bus, address, 255);
        REQUIRE(static_cast<int>(bus.last_address) == 10 << 1);
        REQUIRE(static_cast<int>(bus.last_data) == 254);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }
}

TEST_CASE("DALI Direct Control Commands 0-9") {
    Testbus bus;
    const auto address = libdali::Address::from_short_address(10); // Short address 10 -> 0x15 for command

    SECTION("Command 0: OFF") {
        libdali::Off(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x00);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 1: UP") {
        libdali::Up(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x01);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 2: DOWN") {
        libdali::Down(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x02);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 3: STEP UP") {
        libdali::StepUp(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x03);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 4: STEP DOWN") {
        libdali::StepDown(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x04);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 5: RECALL MAX LEVEL") {
        libdali::RecallMaxLevel(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x05);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 6: RECALL MIN LEVEL") {
        libdali::RecallMinLevel(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x06);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 7: STEP DOWN AND OFF") {
        libdali::StepDownAndOff(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x07);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 8: ON AND STEP UP") {
        libdali::OnAndStepUp(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x08);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }

    SECTION("Command 9: ENABLE DAPC SEQUENCE") {
        libdali::EnableDAPCSequence(&bus, address);
        REQUIRE(static_cast<int>(bus.last_address) == ((10 << 1) | 0x01));
        REQUIRE(static_cast<int>(bus.last_data) == 0x09);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
    }
}


TEST_CASE("251 Terminate") {
  Testbus bus;
  libdali::Terminate(&bus);
  REQUIRE(static_cast<int>(bus.last_address) == 0xa1);
  REQUIRE(static_cast<int>(bus.last_data) == 0x00);
  REQUIRE(static_cast<int>(bus.last_reply_length) == 0);
}

TEST_CASE("252: Query operating mode") {
    Testbus bus;

    SECTION("success 0xff") {
        uint8_t next_reply = 0xff;
        bus.next_reply = &next_reply;
        bus.next_reply_length = 1;
        const auto address = libdali::Address::from_short_address(10);
        auto result = libdali::QueryOperatingMode(&bus, address);
        REQUIRE(result);
        REQUIRE(static_cast<int>(bus.last_address) == 0x15); // 10 << 1 + 1
        REQUIRE(static_cast<int>(bus.last_data) == 252);
        REQUIRE(static_cast<int>(bus.last_reply_length) == 1);
        CHECK(result.value().pwm_active() == true);
        CHECK(result.value().am_active() == true);
        CHECK(result.value().output_current_regulated() == true);
        CHECK(result.value().high_current_pulse_active() == true);
        CHECK(result.value().non_logarithmic_dimming_active() == true);
    }

    SECTION("timeout") {
        bus.next_error_code = libdali::ErrorCode::TIMEOUT;
        const auto address = libdali::Address::from_short_address(10);
        auto result = libdali::QueryOperatingMode(&bus, address);
        REQUIRE(!result);
    }
}
