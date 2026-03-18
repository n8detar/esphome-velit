#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

enum VelitSwitchKind : uint8_t {
  SWITCH_KIND_SWING = 0,
};

class VelitSwitch : public switch_::Switch, public Component, public VelitClient {
 public:
  void dump_config() override;
  void on_velit_state(const VelitState &state) override;

  void set_switch_kind(VelitSwitchKind kind) { this->kind_ = kind; }

 protected:
  void write_state(bool state) override;

  VelitSwitchKind kind_{SWITCH_KIND_SWING};
};

}  // namespace esphome::velit

#endif
