#include "velit_sensor.h"

#ifdef USE_ESP32

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::velit {

static const char *const TAG = "velit.sensor";

void VelitSensor::dump_config() {
  LOG_SENSOR("", "Velit Sensor", this);
}

void VelitSensor::on_velit_state(const VelitState &state) {
  switch (this->kind_) {
    case SENSOR_KIND_AMBIENT_TEMPERATURE:
      this->publish_state(state.current_temperature_c);
      break;
    case SENSOR_KIND_FAULT_CODE:
      this->publish_state(state.fault_code);
      break;
    case SENSOR_KIND_ALTITUDE:
      this->publish_state(state.altitude_valid ? state.altitude_m : NAN);
      break;
  }
}

}  // namespace esphome::velit

#endif
