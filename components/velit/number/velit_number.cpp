#include "velit_number.h"

#ifdef USE_ESP32

#include <cmath>

#include "esphome/core/log.h"

#include "../velit_hub.h"

namespace esphome::velit {

static const char *const TAG = "velit.number";

void VelitNumber::dump_config() {
  LOG_NUMBER("", "Velit Number", this);
}

void VelitNumber::on_velit_state(const VelitState &state) {
  if (this->kind_ != NUMBER_KIND_MANUAL_FAN_SPEED ||
      this->parent_->get_device_type() != DEVICE_TYPE_HEATER) {
    return;
  }
  this->publish_state(state.fan_speed);
}

void VelitNumber::control(float value) {
  if (this->kind_ != NUMBER_KIND_MANUAL_FAN_SPEED ||
      this->parent_->get_device_type() != DEVICE_TYPE_HEATER) {
    return;
  }
  this->parent_->set_heater_manual_fan_speed(
      static_cast<uint8_t>(std::lround(value))
  );
}

}  // namespace esphome::velit

#endif
