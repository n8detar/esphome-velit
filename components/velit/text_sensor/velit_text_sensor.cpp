#include "velit_text_sensor.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"

#include "../velit_hub.h"
#include "../velit_protocol.h"

namespace esphome::velit {

static const char *const TAG = "velit.text_sensor";

void VelitTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Velit Text Sensor", this);
}

void VelitTextSensor::on_velit_state(const VelitState &state) {
  if (this->kind_ != TEXT_SENSOR_KIND_FAULT_TEXT) {
    return;
  }

  if (this->parent_->get_device_type() == DEVICE_TYPE_AC) {
    this->publish_state(ac_fault_text(state.fault_code));
    return;
  }

  this->publish_state(heater_fault_text(state.fault_code));
}

}  // namespace esphome::velit

#endif
