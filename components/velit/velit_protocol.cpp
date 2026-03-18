#include "velit_protocol.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/helpers.h"

namespace esphome::velit {

namespace {

uint8_t low_byte_checksum(const std::vector<uint8_t> &frame) {
  uint32_t sum = 0;
  for (const auto byte : frame) {
    sum += byte;
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

uint16_t full_checksum(const std::vector<uint8_t> &frame) {
  uint32_t sum = 0;
  for (const auto byte : frame) {
    sum += byte;
  }
  return static_cast<uint16_t>(sum & 0xFFFF);
}

float decode_ac_temperature_c(uint8_t raw) {
  if (raw >= 16 && raw <= 30) {
    return raw;
  }
  if (raw >= 60 && raw <= 99) {
    return fahrenheit_to_celsius(static_cast<float>(raw));
  }
  return NAN;
}

float decode_heater_target_temperature_c(uint8_t raw) {
  if (raw >= 4 && raw <= 38) {
    return raw;
  }
  if (raw >= 40 && raw <= 99) {
    return fahrenheit_to_celsius(static_cast<float>(raw));
  }
  return NAN;
}

uint16_t read_be_u16(const uint8_t *value, size_t offset, size_t length) {
  if (offset + 1 >= length) {
    return 0;
  }
  return static_cast<uint16_t>(value[offset] << 8 | value[offset + 1]);
}

}  // namespace

std::vector<uint8_t> build_ac_command(uint8_t command, uint8_t value) {
  std::vector<uint8_t> frame{0x5A, 0x5A, 0x06, 0x01, command, value};
  frame.push_back(low_byte_checksum(frame));
  frame.push_back(0x0D);
  frame.push_back(0x0A);
  return frame;
}

std::vector<uint8_t> build_heater_short_command(uint8_t command, uint8_t value) {
  std::vector<uint8_t> frame{
      0x55, 0x0C, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x2D, command, value,
  };
  frame.push_back(0x00);
  frame.push_back(low_byte_checksum(frame));
  return frame;
}

std::vector<uint8_t> build_heater_clock_sync_command(
    int year, int month, int day, int hour, int minute, int second
) {
  std::vector<uint8_t> frame{
      0x55,
      0x11,
      0x00,
      0x00,
      0x00,
      0x01,
      0x00,
      0x00,
      0x00,
      0x2D,
      0x10,
      static_cast<uint8_t>(hour & 0xFF),
      static_cast<uint8_t>(minute & 0xFF),
      static_cast<uint8_t>(second & 0xFF),
      static_cast<uint8_t>(year & 0xFF),
      static_cast<uint8_t>(month & 0xFF),
      static_cast<uint8_t>(day & 0xFF),
  };
  const auto checksum = full_checksum(frame);
  frame.push_back(static_cast<uint8_t>((checksum >> 8) & 0xFF));
  frame.push_back(static_cast<uint8_t>(checksum & 0xFF));
  return frame;
}

bool parse_ac_notification(const uint8_t *value, uint16_t length, VelitState &state) {
  if (length != 9 || value[0] != 0x5A || value[1] != 0x5A || value[7] != 0x0D ||
      value[8] != 0x0A) {
    return false;
  }

  const auto command = value[4];
  const auto data = value[5];

  switch (command) {
    case 0x01:
      state.power_on = data == 0x02;
      break;
    case 0x02:
      state.ac_mode_code = data;
      break;
    case 0x03: {
      const auto temp = decode_ac_temperature_c(data);
      if (!std::isnan(temp)) {
        state.target_temperature_c = temp;
      }
      break;
    }
    case 0x04:
      state.fan_speed = std::clamp<uint8_t>(data, 1, 5);
      break;
    case 0x07: {
      const auto temp = decode_ac_temperature_c(data);
      if (!std::isnan(temp)) {
        state.current_temperature_c = temp;
      }
      break;
    }
    case 0x10:
      state.swing_on = data == 0x01;
      break;
    case 0x0B:
      state.fault_code = data;
      break;
    default:
      return false;
  }

  return true;
}

bool parse_heater_notification(
    const uint8_t *value, uint16_t length, VelitState &state
) {
  if (length < 14 || value[0] != 0x55) {
    return false;
  }

  if (length == 22) {
    state.fault_code = value[13];
    state.heater_operating_mode =
        value[14] == 0x01 ? HEATER_OPERATING_MODE_MANUAL
                          : HEATER_OPERATING_MODE_THERMOSTAT;
    state.fan_speed = std::clamp<uint8_t>(value[15], 1, 5);
    const auto target_temperature = decode_heater_target_temperature_c(value[16]);
    if (!std::isnan(target_temperature)) {
      state.target_temperature_c = target_temperature;
    }
    state.heater_other_state = value[17];
    state.power_on = state.heater_other_state != 0x00;
    return true;
  }

  if (length == 28) {
    state.fault_code = value[13];
    const auto ambient_temp_f = read_be_u16(value, 18, length);
    if (ambient_temp_f <= 700) {
      state.current_temperature_c =
          fahrenheit_to_celsius(static_cast<float>(ambient_temp_f) - 60.0f);
    }
    state.altitude_m = read_be_u16(value, 24, length);
    state.altitude_valid = true;
    return true;
  }

  if (length == 20 && value[12] == 0x0F) {
    return true;
  }

  if (length == 16 || length == 24) {
    return true;
  }

  return false;
}

const char *heater_fault_text(uint8_t code) {
  switch (code) {
    case 0:
      return "";
    case 1:
      return "Ignition failure";
    case 2:
      return "Combustion interruption";
    case 3:
      return "Power supply voltage";
    case 4:
      return "Heat exchanger overheat";
    case 5:
      return "Ignition sensor";
    case 6:
      return "Outlet temperature sensor";
    case 7:
      return "Fuel pump";
    case 8:
      return "Fan motor";
    case 9:
      return "Inlet temperature sensor";
    case 10:
      return "Glow plug";
    case 11:
      return "Environment temperature";
    case 12:
      return "Elevation too high or too low";
    case 13:
      return "Fan speed abnormal";
    case 14:
      return "Carbon Monoxide high";
    case 15:
      return "LIN Communication failure";
    default:
      return "";
  }
}

std::string ac_fault_text(uint8_t code) {
  if (code == 0) {
    return "";
  }
  const char hex[] = "0123456789ABCDEF";
  std::string result;
  result.push_back(hex[(code >> 4) & 0x0F]);
  result.push_back(hex[code & 0x0F]);
  return result;
}

}  // namespace esphome::velit
