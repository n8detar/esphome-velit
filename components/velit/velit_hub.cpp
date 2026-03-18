#include "velit_hub.h"

#ifdef USE_ESP32

#include <algorithm>
#include <cmath>
#include <ctime>

#include "esphome/core/log.h"

namespace esphome::velit {

static const char *const TAG = "velit";

namespace {

uint8_t clamp_ac_temperature(float temperature_c) {
  return static_cast<uint8_t>(std::lround(std::clamp(temperature_c, 16.0f, 30.0f)));
}

uint8_t clamp_heater_temperature(float temperature_c) {
  return static_cast<uint8_t>(std::lround(std::clamp(temperature_c, 4.0f, 38.0f)));
}

uint8_t clamp_fan_speed(uint8_t fan_speed) {
  return std::clamp<uint8_t>(fan_speed, 1, 5);
}

uint8_t heater_mode_to_code(HeaterOperatingMode mode) {
  return mode == HEATER_OPERATING_MODE_MANUAL ? 0x01 : 0x02;
}

}  // namespace

void VelitHub::setup() {
  this->state_ = {};
}

void VelitHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Velit Hub");
  ESP_LOGCONFIG(TAG, "  Device Type: %s", this->device_type_ == DEVICE_TYPE_AC ? "AC" : "Heater");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->parent_->address_str());
  LOG_UPDATE_INTERVAL(this);
}

void VelitHub::register_child(VelitClient *child) {
  this->children_.push_back(child);
}

uint32_t VelitHub::tx_gap_ms_() const {
  return this->device_type_ == DEVICE_TYPE_AC ? 300 : 100;
}

uint32_t VelitHub::refresh_delay_ms_() const {
  return this->device_type_ == DEVICE_TYPE_AC ? 1000 : 4000;
}

void VelitHub::clear_queues_() {
  this->user_queue_.clear();
  this->poll_queue_.clear();
  this->next_tx_at_ = 0;
  this->pending_refresh_at_ = 0;
  this->refresh_pending_ = false;
}

void VelitHub::dispatch_state_() {
  for (auto *child : this->children_) {
    child->on_velit_state(this->state_);
  }
}

bool VelitHub::discover_characteristics_() {
  auto *notify_chr = this->parent_->get_characteristic(VELIT_SERVICE_UUID, VELIT_NOTIFY_UUID);
  if (notify_chr == nullptr) {
    ESP_LOGW(TAG, "[%s] Notify characteristic not found", this->parent_->address_str());
    return false;
  }

  auto *write_chr = this->parent_->get_characteristic(VELIT_SERVICE_UUID, VELIT_WRITE_UUID);
  if (write_chr == nullptr) {
    ESP_LOGW(TAG, "[%s] Write characteristic not found", this->parent_->address_str());
    return false;
  }

  this->notify_char_handle_ = notify_chr->handle;
  this->write_char_handle_ = write_chr->handle;

  auto status = esp_ble_gattc_register_for_notify(
      this->parent_->get_gattc_if(),
      this->parent_->get_remote_bda(),
      this->notify_char_handle_
  );
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "[%s] Failed to register for notify, status=%d", this->parent_->address_str(), status);
    return false;
  }

  return true;
}

bool VelitHub::write_frame_(const std::vector<uint8_t> &frame) {
  if (this->node_state != espbt::ClientState::ESTABLISHED || this->write_char_handle_ == 0) {
    return false;
  }

  auto status = esp_ble_gattc_write_char(
      this->parent_->get_gattc_if(),
      this->parent_->get_conn_id(),
      this->write_char_handle_,
      static_cast<uint16_t>(frame.size()),
      const_cast<uint8_t *>(frame.data()),
      ESP_GATT_WRITE_TYPE_NO_RSP,
      ESP_GATT_AUTH_REQ_NONE
  );
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "[%s] Failed writing frame, status=%d", this->parent_->address_str(), status);
    return false;
  }

  return true;
}

void VelitHub::enqueue_user_frame_(std::vector<uint8_t> frame) {
  this->user_queue_.push_back({std::move(frame)});
}

void VelitHub::schedule_refresh_(uint32_t delay_ms) {
  this->pending_refresh_at_ = millis() + delay_ms;
  this->refresh_pending_ = true;
}

void VelitHub::replace_poll_queue_(const std::vector<std::vector<uint8_t>> &frames) {
  this->poll_queue_.clear();
  for (const auto &frame : frames) {
    this->poll_queue_.push_back({frame});
  }
}

void VelitHub::queue_full_sync_() {
  if (this->device_type_ == DEVICE_TYPE_AC) {
    this->replace_poll_queue_({
        build_ac_command(0x01, 0x00),
        build_ac_command(0x02, 0x00),
        build_ac_command(0x03, 0x00),
        build_ac_command(0x04, 0x00),
        build_ac_command(0x07, 0x00),
        build_ac_command(0x10, 0x00),
        build_ac_command(0x0B, 0x00),
    });
  } else {
    this->replace_poll_queue_({
        build_heater_short_command(0x0A, 0x00),
        build_heater_short_command(0x0B, 0x00),
        build_heater_short_command(0x0F, 0x00),
    });
  }
}

void VelitHub::queue_connect_sync_() {
  this->clear_queues_();

  if (this->device_type_ == DEVICE_TYPE_HEATER) {
    std::time_t now = std::time(nullptr);
    if (now > 1577836800) {
      std::tm time_info{};
      localtime_r(&now, &time_info);
      this->enqueue_user_frame_(build_heater_clock_sync_command(
          (time_info.tm_year + 1900) % 100,
          time_info.tm_mon + 1,
          time_info.tm_mday,
          time_info.tm_hour,
          time_info.tm_min,
          time_info.tm_sec
      ));
    } else {
      ESP_LOGW(TAG, "Skipping heater clock sync because system time is unavailable");
    }
  }

  this->queue_full_sync_();
}

void VelitHub::handle_notification_(const uint8_t *value, uint16_t length) {
  bool handled = false;
  if (this->device_type_ == DEVICE_TYPE_AC) {
    handled = parse_ac_notification(value, length, this->state_);
  } else {
    handled = parse_heater_notification(value, length, this->state_);
  }

  if (handled) {
    this->dispatch_state_();
  }
}

void VelitHub::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    return;
  }
  this->queue_full_sync_();
}

void VelitHub::loop() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    return;
  }

  const auto now = millis();

  if (this->refresh_pending_ && this->user_queue_.empty() && now >= this->pending_refresh_at_) {
    this->refresh_pending_ = false;
    this->queue_full_sync_();
  }

  if (now < this->next_tx_at_) {
    return;
  }

  if (!this->user_queue_.empty()) {
    const auto frame = this->user_queue_.front().data;
    this->user_queue_.pop_front();
    if (this->write_frame_(frame)) {
      this->next_tx_at_ = now + this->tx_gap_ms_();
    }
    return;
  }

  if (!this->poll_queue_.empty()) {
    const auto frame = this->poll_queue_.front().data;
    this->poll_queue_.pop_front();
    if (this->write_frame_(frame)) {
      this->next_tx_at_ = now + this->tx_gap_ms_();
    }
  }
}

void VelitHub::gattc_event_handler(
    esp_gattc_cb_event_t event,
    esp_gatt_if_t gattc_if,
    esp_ble_gattc_cb_param_t *param
) {
  switch (event) {
    case ESP_GATTC_DISCONNECT_EVT:
      this->state_.connected = false;
      this->write_char_handle_ = 0;
      this->notify_char_handle_ = 0;
      this->clear_queues_();
      this->status_set_warning();
      this->dispatch_state_();
      break;
    case ESP_GATTC_SEARCH_CMPL_EVT:
      if (!this->discover_characteristics_()) {
        this->status_set_warning();
      }
      break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
      if (param->reg_for_notify.handle == this->notify_char_handle_ &&
          param->reg_for_notify.status == ESP_GATT_OK) {
        this->node_state = espbt::ClientState::ESTABLISHED;
        this->state_.connected = true;
        this->status_clear_warning();
        this->dispatch_state_();
        this->queue_connect_sync_();
      }
      break;
    case ESP_GATTC_NOTIFY_EVT:
      if (param->notify.handle == this->notify_char_handle_) {
        this->handle_notification_(param->notify.value, param->notify.value_len);
      }
      break;
    default:
      break;
  }
}

bool VelitHub::set_ac_power(bool on) {
  if (this->device_type_ != DEVICE_TYPE_AC) {
    return false;
  }
  this->enqueue_user_frame_(build_ac_command(0x01, on ? 0x02 : 0x01));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_ac_mode(uint8_t mode_code) {
  if (this->device_type_ != DEVICE_TYPE_AC) {
    return false;
  }
  this->enqueue_user_frame_(build_ac_command(0x02, mode_code));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_ac_target_temperature(float temperature_c) {
  if (this->device_type_ != DEVICE_TYPE_AC) {
    return false;
  }
  this->enqueue_user_frame_(build_ac_command(0x03, clamp_ac_temperature(temperature_c)));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_ac_fan_speed(uint8_t fan_speed) {
  if (this->device_type_ != DEVICE_TYPE_AC) {
    return false;
  }
  this->enqueue_user_frame_(build_ac_command(0x04, clamp_fan_speed(fan_speed)));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_ac_swing(bool on) {
  if (this->device_type_ != DEVICE_TYPE_AC) {
    return false;
  }
  this->enqueue_user_frame_(build_ac_command(0x10, on ? 0x01 : 0x02));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_heater_power(bool on) {
  if (this->device_type_ != DEVICE_TYPE_HEATER) {
    return false;
  }
  if (on) {
    this->enqueue_user_frame_(build_heater_short_command(
        0x01, heater_mode_to_code(this->state_.heater_operating_mode)
    ));
  } else {
    this->enqueue_user_frame_(build_heater_short_command(0x02, 0x00));
  }
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_heater_operating_mode(HeaterOperatingMode mode) {
  if (this->device_type_ != DEVICE_TYPE_HEATER) {
    return false;
  }
  this->enqueue_user_frame_(build_heater_short_command(0x00, heater_mode_to_code(mode)));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_heater_target_temperature(float temperature_c) {
  if (this->device_type_ != DEVICE_TYPE_HEATER) {
    return false;
  }
  this->enqueue_user_frame_(build_heater_short_command(
      0x08, clamp_heater_temperature(temperature_c)
  ));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

bool VelitHub::set_heater_manual_fan_speed(uint8_t fan_speed) {
  if (this->device_type_ != DEVICE_TYPE_HEATER) {
    return false;
  }
  this->enqueue_user_frame_(build_heater_short_command(0x07, clamp_fan_speed(fan_speed)));
  this->schedule_refresh_(this->refresh_delay_ms_());
  return true;
}

}  // namespace esphome::velit

#endif
