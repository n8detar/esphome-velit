#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

enum VelitNumberKind : uint8_t {
  NUMBER_KIND_MANUAL_FAN_SPEED = 0,
};

class VelitNumber : public number::Number, public Component, public VelitClient {
 public:
  void dump_config() override;
  void on_velit_state(const VelitState &state) override;

  void set_number_kind(VelitNumberKind kind) { this->kind_ = kind; }

 protected:
  void control(float value) override;

  VelitNumberKind kind_{NUMBER_KIND_MANUAL_FAN_SPEED};
};

}  // namespace esphome::velit

#endif
