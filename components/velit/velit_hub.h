#pragma once

#include <deque>
#include <vector>

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/component.h"

#include "velit_child.h"
#include "velit_protocol.h"
#include "velit_state.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome::velit {

namespace espbt = esphome::esp32_ble_tracker;

class VelitHub : public ble_client::BLEClientNode, public PollingComponent {
 public:
  VelitHub() = default;

  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::BLUETOOTH; }

  void gattc_event_handler(
      esp_gattc_cb_event_t event,
      esp_gatt_if_t gattc_if,
      esp_ble_gattc_cb_param_t *param
  ) override;

  void register_child(VelitClient *child);

  void set_device_type(VelitDeviceType type) { this->device_type_ = type; }
  VelitDeviceType get_device_type() const { return this->device_type_; }
  const VelitState &state() const { return this->state_; }

  bool set_ac_power(bool on);
  bool set_ac_mode(uint8_t mode_code);
  bool set_ac_target_temperature(float temperature_c);
  bool set_ac_fan_speed(uint8_t fan_speed);
  bool set_ac_swing(bool on);

  bool set_heater_power(bool on);
  bool set_heater_operating_mode(HeaterOperatingMode mode);
  bool set_heater_target_temperature(float temperature_c);
  bool set_heater_manual_fan_speed(uint8_t fan_speed);

 protected:
  struct TxFrame {
    std::vector<uint8_t> data;
  };

  bool discover_characteristics_();
  void dispatch_state_();
  void handle_notification_(const uint8_t *value, uint16_t length);
  bool write_frame_(const std::vector<uint8_t> &frame);
  void queue_connect_sync_();
  void queue_full_sync_();
  void replace_poll_queue_(const std::vector<std::vector<uint8_t>> &frames);
  void enqueue_user_frame_(std::vector<uint8_t> frame);
  void schedule_refresh_(uint32_t delay_ms);
  void clear_queues_();

  uint32_t tx_gap_ms_() const;
  uint32_t refresh_delay_ms_() const;

  std::vector<VelitClient *> children_;
  std::deque<TxFrame> user_queue_;
  std::deque<TxFrame> poll_queue_;
  VelitState state_{};
  VelitDeviceType device_type_{DEVICE_TYPE_AC};
  uint16_t write_char_handle_{0};
  uint16_t notify_char_handle_{0};
  uint32_t next_tx_at_{0};
  uint32_t pending_refresh_at_{0};
  bool refresh_pending_{false};
};

}  // namespace esphome::velit

#endif
