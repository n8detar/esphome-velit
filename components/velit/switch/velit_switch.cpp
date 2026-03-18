#include "velit_switch.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"

#include "../velit_hub.h"

namespace esphome::velit {

static const char *const TAG = "velit.switch";

void VelitSwitch::dump_config() {
  LOG_SWITCH("", "Velit Switch", this);
}

void VelitSwitch::on_velit_state(const VelitState &state) {
  if (this->kind_ != SWITCH_KIND_SWING ||
      this->parent_->get_device_type() != DEVICE_TYPE_AC) {
    return;
  }
  this->publish_state(state.swing_on);
}

void VelitSwitch::write_state(bool state) {
  if (this->kind_ != SWITCH_KIND_SWING ||
      this->parent_->get_device_type() != DEVICE_TYPE_AC) {
    return;
  }
  this->parent_->set_ac_swing(state);
}

}  // namespace esphome::velit

#endif
