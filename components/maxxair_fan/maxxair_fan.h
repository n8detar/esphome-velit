#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
namespace maxxair_fan {

static const char *const TAG = "maxxair_fan";

// ─────────────────────────────────────────────────────────────────────────────
// MaxxAir / Maxxfan IR Protocol
//
// Protocol reverse-engineered by skypeachblue, wingspinner, and Jeff Brown
// (https://github.com/brown-studios/esphome-maxxfan-protocol).
//
// The signal is RS232-like at 38 kHz carrier:
//   • Each bit period = 800 µs
//   • Mark (IR on)  = logic 0
//   • Space (IR off) = logic 1
//   • Frame: 1 start bit (0), 8 data bits LSB-first, 2 stop bits (1,1)
//   • 16 bytes total: 10-byte preamble + 4 control bytes + 1 checksum byte
//   • Packet ends with a long space
//
// Preamble (fixed, bytes 0-9):
//   0x5A, 0xA5, 0x80, 0x7F, 0x40, 0xBF, 0x20, 0xDF, 0x10, 0xCC
//
// Control (bytes 10-13):
//   [10] state byte  — flags (see below)
//   [11] speed byte  — fan speed percent (0, 10, 20 … 100)
//   [12] auto_temp   — thermostat setpoint in °F (29-99), default 78
//   [13] 0xFF        — always 0xFF
//   [14] 0x23        — always 0x23
//   [15] checksum    — XOR of bytes 10-14
//
// State byte flags (LSB first):
//   bit 0  fan_on      1 = fan on
//   bit 1  special     1 = ceiling fan mode OR auto mode active
//   bit 2  fan_exhaust 1 = exhaust (air out), 0 = intake (air in)
//   bit 3  cover_open  1 = cover open
//   bit 4  auto_mode   1 = thermostat auto mode
//   bit 5  warn        1 = beep twice (out-of-range warning)
//   bit 6  (reserved)  always 0
//   bit 7  (reserved)  always 0
// ─────────────────────────────────────────────────────────────────────────────

static const uint8_t MAXXFAN_PREAMBLE[10] = {
  0x5A, 0xA5, 0x80, 0x7F, 0x40, 0xBF, 0x20, 0xDF, 0x10, 0xCC
};

static constexpr uint32_t MAXXFAN_BIT_PERIOD_US = 800;
static constexpr uint32_t MAXXFAN_END_SPACE_US   = 8000;

struct MaxxfanData {
  bool    fan_on           = false;
  bool    fan_exhaust      = true;   // true = exhaust/out, false = intake/in
  bool    cover_open       = false;
  bool    auto_mode        = false;
  bool    special          = false;  // ceiling fan mode or auto mode flag
  bool    warn             = false;
  uint8_t fan_speed        = 10;     // 10-100 in steps of 10
  uint8_t auto_temperature = 78;     // °F, 29-99
};

// ─────────────────────────────────────────────────────────────────────────────
// MaxxAirFan component
// ─────────────────────────────────────────────────────────────────────────────

class MaxxAirFan : public Component {
 public:
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *tx) {
    this->transmitter_ = tx;
  }

  // High-level helpers called from lambdas in the YAML
  void send_exhaust(uint8_t speed_1_to_10) {
    MaxxfanData d;
    d.fan_on      = true;
    d.fan_exhaust = true;
    d.cover_open  = true;
    d.fan_speed   = speed_1_to_10 * 10;
    this->transmit_(d);
  }

  void send_intake(uint8_t speed_1_to_10) {
    MaxxfanData d;
    d.fan_on      = true;
    d.fan_exhaust = false;
    d.cover_open  = true;
    d.fan_speed   = speed_1_to_10 * 10;
    this->transmit_(d);
  }

  void send_ceiling(uint8_t speed_1_to_10) {
    // Ceiling fan mode: fan on, lid closed, special flag set
    MaxxfanData d;
    d.fan_on      = true;
    d.fan_exhaust = false;
    d.cover_open  = false;
    d.special     = true;
    d.fan_speed   = speed_1_to_10 * 10;
    this->transmit_(d);
  }

  void send_lid_open() {
    MaxxfanData d;
    d.fan_on     = false;
    d.cover_open = true;
    this->transmit_(d);
  }

  void send_lid_close() {
    MaxxfanData d;
    d.fan_on     = false;
    d.cover_open = false;
    this->transmit_(d);
  }

  // Full-control method — available to users via lambdas for advanced use
  void send(bool fan_on, bool fan_exhaust, bool cover_open,
            uint8_t speed_pct, bool auto_mode = false,
            uint8_t auto_temperature = 78, bool special = false,
            bool warn = false) {
    MaxxfanData d;
    d.fan_on           = fan_on;
    d.fan_exhaust      = fan_exhaust;
    d.cover_open       = cover_open;
    d.fan_speed        = speed_pct;
    d.auto_mode        = auto_mode;
    d.auto_temperature = auto_temperature;
    d.special          = special || auto_mode;  // special must be set when auto_mode is on
    d.warn             = warn;
    this->transmit_(d);
  }

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};

  // Encode a MaxxfanData packet and transmit it
  void transmit_(const MaxxfanData &data);

  // Append one RS232-framed byte to the mark/space buffer
  void encode_byte_(remote_base::RemoteTransmitData *out, uint8_t byte_val);
};

}  // namespace maxxair_fan
}  // namespace esphome
