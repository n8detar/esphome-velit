#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include "../velit_child.h"

#ifdef USE_ESP32

namespace esphome::velit {

enum VelitSelectKind : uint8_t {
  SELECT_KIND_OPERATING_MODE = 0,
};

class VelitSelect : public select::Select, public Component, public VelitClient {
 public:
  void dump_config() override;
  void on_velit_state(const VelitState &state) override;

  void set_select_kind(VelitSelectKind kind) { this->kind_ = kind; }

 protected:
  void control(size_t index) override;

  VelitSelectKind kind_{SELECT_KIND_OPERATING_MODE};
};

}  // namespace esphome::velit

#endif
