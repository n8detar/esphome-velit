#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

enum VelitTextSensorKind : uint8_t {
  TEXT_SENSOR_KIND_FAULT_TEXT = 0,
};

class VelitTextSensor : public text_sensor::TextSensor, public Component, public VelitClient {
 public:
  void dump_config() override;
  void on_velit_state(const VelitState &state) override;

  void set_text_sensor_kind(VelitTextSensorKind kind) { this->kind_ = kind; }

 protected:
  VelitTextSensorKind kind_{TEXT_SENSOR_KIND_FAULT_TEXT};
};

}  // namespace esphome::velit

#endif
