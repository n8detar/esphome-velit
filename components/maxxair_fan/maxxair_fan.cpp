#include "maxxair_fan.h"
#include "esphome/core/log.h"

namespace esphome {
namespace maxxair_fan {

void MaxxAirFan::dump_config() {
  ESP_LOGCONFIG(TAG, "MaxxAir Fan IR Component:");
  if (this->transmitter_ == nullptr) {
    ESP_LOGE(TAG, "  No IR transmitter configured!");
  } else {
    ESP_LOGCONFIG(TAG, "  IR transmitter: configured");
  }
}

// ---------------------------------------------------------------------------
// encode_byte_
//
// Serialises one byte as RS232 framing into mark/space IR timings.
//   Frame: [start=mark] [b0..b7, LSB-first, mark=0 space=1] [stop1=space] [stop2=space]
//   Each symbol is exactly MAXXFAN_BIT_PERIOD_US wide.
//
// The RemoteTransmitData API accumulates consecutive marks and spaces, so we
// must use a temporary buffer approach: we build an alternating array and
// consolidate same-polarity runs.
// ---------------------------------------------------------------------------
void MaxxAirFan::encode_byte_(remote_base::RemoteTransmitData *out, uint8_t byte_val) {
  // We build 11 symbols: 1 start + 8 data + 2 stop
  // mark  = IR on  = +duration
  // space = IR off = -duration (encoded as positive; RemoteTransmitData handles polarity)

  // Symbol sequence: true = mark, false = space
  bool symbols[11];
  symbols[0] = true;  // start bit = mark (logic 0)
  for (int i = 0; i < 8; i++) {
    // LSB first; mark = 0, space = 1
    symbols[1 + i] = !((byte_val >> i) & 1);
  }
  symbols[9]  = false;  // stop bit 1 = space (logic 1)
  symbols[10] = false;  // stop bit 2 = space (logic 1)

  // Consolidate into the RemoteTransmitData buffer
  for (int i = 0; i < 11; i++) {
    if (symbols[i]) {
      out->mark(MAXXFAN_BIT_PERIOD_US);
    } else {
      out->space(MAXXFAN_BIT_PERIOD_US);
    }
  }
}

// ---------------------------------------------------------------------------
// transmit_
//
// Builds the 16-byte Maxxfan packet, serialises it, and fires the IR LED.
//
// Packet layout (16 bytes):
//   bytes  0-9  : fixed preamble  (MAXXFAN_PREAMBLE)
//   byte  10    : state flags
//   byte  11    : fan speed percent (0, 10, 20 … 100)
//   byte  12    : auto temperature setpoint (°F)
//   byte  13    : 0xFF  (always)
//   byte  14    : 0x23  (always)
//   byte  15    : checksum = XOR of bytes 10-14
//   end         : long trailing space
// ---------------------------------------------------------------------------
void MaxxAirFan::transmit_(const MaxxfanData &data) {
  if (this->transmitter_ == nullptr) {
    ESP_LOGE(TAG, "No IR transmitter configured — cannot send");
    return;
  }

  // ── Build the 16-byte packet ──────────────────────────────────────────────
  uint8_t packet[16] = {};

  // Preamble
  for (int i = 0; i < 10; i++) {
    packet[i] = MAXXFAN_PREAMBLE[i];
  }

  // State byte (byte 10)
  uint8_t state = 0;
  if (data.fan_on)      state |= (1 << 0);
  if (data.special)     state |= (1 << 1);
  if (data.fan_exhaust) state |= (1 << 2);
  if (data.cover_open)  state |= (1 << 3);
  if (data.auto_mode)   state |= (1 << 4);
  if (data.warn)        state |= (1 << 5);
  packet[10] = state;

  // Speed byte (byte 11) — 0 to 100 in multiples of 10
  uint8_t spd = data.fan_speed;
  if (spd > 100) spd = 100;
  // Round to nearest 10
  spd = ((spd + 5) / 10) * 10;
  packet[11] = spd;

  // Auto temperature (byte 12)
  uint8_t temp = data.auto_temperature;
  if (temp < 29) temp = 29;
  if (temp > 99) temp = 99;
  packet[12] = temp;

  // Fixed bytes
  packet[13] = 0xFF;
  packet[14] = 0x23;

  // Checksum (byte 15) = XOR of bytes 10-14
  packet[15] = packet[10] ^ packet[11] ^ packet[12] ^ packet[13] ^ packet[14];

  ESP_LOGD(TAG, "Maxxfan TX: on=%d exhaust=%d open=%d special=%d auto=%d speed=%d temp=%dF",
           data.fan_on, data.fan_exhaust, data.cover_open,
           data.special, data.auto_mode, spd, temp);
  ESP_LOGV(TAG, "  Packet: %02X %02X %02X %02X %02X %02X",
           packet[10], packet[11], packet[12], packet[13], packet[14], packet[15]);

  // ── Serialise and transmit ────────────────────────────────────────────────
  auto call = this->transmitter_->transmit();
  auto *out = call.get_data();
  out->set_carrier_frequency(38000);
  // Reserve space: 16 bytes × 11 symbols = 176 entries + 1 trailing space
  out->reserve(177);

  for (int i = 0; i < 16; i++) {
    this->encode_byte_(out, packet[i]);
  }

  // Trailing long space marks end of transmission
  out->space(MAXXFAN_END_SPACE_US);

  call.perform();
}

}  // namespace maxxair_fan
}  // namespace esphome
