#include "velit_select.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"

#include "../velit_hub.h"

namespace esphome::velit {

static const char *const TAG = "velit.select";

void VelitSelect::dump_config() {
  LOG_SELECT("", "Velit Select", this);
}

void VelitSelect::on_velit_state(const VelitState &state) {
  if (this->kind_ != SELECT_KIND_OPERATING_MODE ||
      this->parent_->get_device_type() != DEVICE_TYPE_HEATER) {
    return;
  }

  this->publish_state(
      state.heater_operating_mode == HEATER_OPERATING_MODE_MANUAL ? 1 : 0
  );
}

void VelitSelect::control(size_t index) {
  if (this->kind_ != SELECT_KIND_OPERATING_MODE ||
      this->parent_->get_device_type() != DEVICE_TYPE_HEATER) {
    return;
  }

  this->parent_->set_heater_operating_mode(
      index == 1 ? HEATER_OPERATING_MODE_MANUAL
                 : HEATER_OPERATING_MODE_THERMOSTAT
  );
}

}  // namespace esphome::velit

#endif
