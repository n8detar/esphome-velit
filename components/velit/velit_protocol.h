#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "velit_state.h"

namespace esphome::velit {

static constexpr uint16_t VELIT_SERVICE_UUID = 0xFFE0;
static constexpr uint16_t VELIT_NOTIFY_UUID = 0xFFE1;
static constexpr uint16_t VELIT_WRITE_UUID = 0xFFE2;

std::vector<uint8_t> build_ac_command(uint8_t command, uint8_t value);
std::vector<uint8_t> build_heater_short_command(uint8_t command, uint8_t value);
std::vector<uint8_t> build_heater_clock_sync_command(
    int year, int month, int day, int hour, int minute, int second
);

bool parse_ac_notification(const uint8_t *value, uint16_t length, VelitState &state);
bool parse_heater_notification(
    const uint8_t *value, uint16_t length, VelitState &state
);

const char *heater_fault_text(uint8_t code);
std::string ac_fault_text(uint8_t code);

}  // namespace esphome::velit
