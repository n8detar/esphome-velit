#include "velit_climate.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"

#include "../velit_hub.h"

namespace esphome::velit {

static const char *const TAG = "velit.climate";
static constexpr const char *AC_FAN_MODE_NAMES[] = {
    "Speed 1", "Speed 2", "Speed 3", "Speed 4", "Speed 5"
};

void VelitClimate::dump_config() {
  LOG_CLIMATE("", "Velit Climate", this);
}

climate::ClimateTraits VelitClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

  if (this->parent_->get_device_type() == DEVICE_TYPE_AC) {
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_supported_custom_fan_modes(AC_FAN_MODE_NAMES);
    traits.set_visual_min_temperature(16.0f);
    traits.set_visual_max_temperature(30.0f);
  } else {
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_HEAT,
    });
    traits.set_visual_min_temperature(4.0f);
    traits.set_visual_max_temperature(38.0f);
  }

  traits.set_visual_temperature_step(1.0f);
  return traits;
}

climate::ClimateMode VelitClimate::ac_mode_from_state_(const VelitState &state) const {
  if (!state.power_on) {
    return climate::CLIMATE_MODE_OFF;
  }
  switch (state.ac_mode_code) {
    case 0x02:
      return climate::CLIMATE_MODE_HEAT;
    case 0x03:
      return climate::CLIMATE_MODE_FAN_ONLY;
    case 0x01:
    default:
      return climate::CLIMATE_MODE_COOL;
  }
}

uint8_t VelitClimate::ac_mode_to_code_(climate::ClimateMode mode) const {
  switch (mode) {
    case climate::CLIMATE_MODE_HEAT:
      return 0x02;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return 0x03;
    case climate::CLIMATE_MODE_COOL:
    default:
      return 0x01;
  }
}

uint8_t VelitClimate::ac_fan_mode_to_speed_(StringRef fan_mode) const {
  for (uint8_t index = 0; index < 5; index++) {
    if (fan_mode == AC_FAN_MODE_NAMES[index]) {
      return static_cast<uint8_t>(index + 1);
    }
  }
  return 1;
}

void VelitClimate::on_velit_state(const VelitState &state) {
  if (this->parent_->get_device_type() == DEVICE_TYPE_AC) {
    this->mode = this->ac_mode_from_state_(state);
    if (state.fan_speed >= 1 && state.fan_speed <= 5) {
      this->set_custom_fan_mode_(AC_FAN_MODE_NAMES[state.fan_speed - 1]);
    } else {
      this->clear_custom_fan_mode_();
    }
  } else {
    this->mode = state.power_on ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
    this->clear_custom_fan_mode_();
  }

  this->current_temperature = state.current_temperature_c;
  this->target_temperature = state.target_temperature_c;
  this->publish_state();
}

void VelitClimate::control(const climate::ClimateCall &call) {
  if (this->parent_->get_device_type() == DEVICE_TYPE_AC) {
    if (call.get_mode().has_value()) {
      const auto mode = *call.get_mode();
      if (mode == climate::CLIMATE_MODE_OFF) {
        this->parent_->set_ac_power(false);
      } else {
        this->parent_->set_ac_mode(this->ac_mode_to_code_(mode));
        this->parent_->set_ac_power(true);
      }
    }
    if (call.get_target_temperature().has_value()) {
      this->parent_->set_ac_target_temperature(*call.get_target_temperature());
    }
    if (call.has_custom_fan_mode()) {
      this->parent_->set_ac_fan_speed(
          this->ac_fan_mode_to_speed_(call.get_custom_fan_mode())
      );
    }
    return;
  }

  if (call.get_mode().has_value()) {
    this->parent_->set_heater_power(*call.get_mode() != climate::CLIMATE_MODE_OFF);
  }
  if (call.get_target_temperature().has_value()) {
    this->parent_->set_heater_target_temperature(*call.get_target_temperature());
  }
}

}  // namespace esphome::velit

#endif
