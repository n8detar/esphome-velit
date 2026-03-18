#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

class VelitClimate : public climate::Climate, public Component, public VelitClient {
 public:
  void dump_config() override;
  climate::ClimateTraits traits() override;
  void on_velit_state(const VelitState &state) override;

 protected:
  void control(const climate::ClimateCall &call) override;

  climate::ClimateMode ac_mode_from_state_(const VelitState &state) const;
  uint8_t ac_mode_to_code_(climate::ClimateMode mode) const;
  uint8_t ac_fan_mode_to_speed_(esphome::StringRef fan_mode) const;
};

}  // namespace esphome::velit

#endif
