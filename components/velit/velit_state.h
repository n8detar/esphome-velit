#pragma once

#include <cstdint>
#include <cmath>

namespace esphome::velit {

enum VelitDeviceType : uint8_t {
  DEVICE_TYPE_AC = 0,
  DEVICE_TYPE_HEATER = 1,
};

enum HeaterOperatingMode : uint8_t {
  HEATER_OPERATING_MODE_THERMOSTAT = 0,
  HEATER_OPERATING_MODE_MANUAL = 1,
};

struct VelitState {
  bool connected{false};
  bool power_on{false};
  float current_temperature_c{NAN};
  float target_temperature_c{NAN};
  uint8_t fan_speed{0};
  uint8_t fault_code{0};
  bool swing_on{false};
  uint8_t ac_mode_code{0};
  HeaterOperatingMode heater_operating_mode{HEATER_OPERATING_MODE_THERMOSTAT};
  uint8_t heater_other_state{0};
  int altitude_m{0};
  bool altitude_valid{false};
};

}  // namespace esphome::velit
