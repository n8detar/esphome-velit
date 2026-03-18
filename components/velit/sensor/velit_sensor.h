#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

enum VelitSensorKind : uint8_t {
  SENSOR_KIND_AMBIENT_TEMPERATURE = 0,
  SENSOR_KIND_FAULT_CODE = 1,
  SENSOR_KIND_ALTITUDE = 2,
};

class VelitSensor : public sensor::Sensor, public Component, public VelitClient {
 public:
  void dump_config() override;
  void on_velit_state(const VelitState &state) override;

  void set_sensor_kind(VelitSensorKind kind) { this->kind_ = kind; }

 protected:
  VelitSensorKind kind_{SENSOR_KIND_AMBIENT_TEMPERATURE};
};

}  // namespace esphome::velit

#endif
