# MaxxAir Fan ESPHome External Component

ESPHome external component for controlling IR-equipped MaxxAir fans (7000K/7500K series) from Home Assistant. It adds a `maxxair_fan:` IR hub plus fan, switch, cover, and sensor entities with full state-machine logic matching the physical remote.

## Important Notes

- This is an unofficial project provided as-is. Use at your own risk.
- IR control is one-way. ESPHome cannot read the physical state of the fan — if someone presses the buttons on the fan itself, Home Assistant will be out of sync until the next command is sent from HA.
- ESPHome ≥ 2025.7.0 is required for sub-device support. The example config will still compile on older versions if you remove the `devices:` block and all `device_id:` lines.
- The IR LED must have a clear line-of-sight to the MaxxAir IR receiver window, or be routed directly into the fan housing.

## Features

- `maxxair_fan:` IR hub bound to a `remote_transmitter`
- Encodes the MaxxAir IR protocol at runtime — no hardcoded raw IR blobs
  - 16-byte RS232-framed packet at 38 kHz / 800 µs per bit
  - Checksum-protected, templatable parameters
- Fan entity
  - 10 speeds
  - Forward (exhaust / air out) and reverse (intake / air in)
- Switch entities
  - `Ceiling Fan Mode` — lid-closed downflow mode, sets the `special` protocol flag
  - `Auto Fan` — state flag for use with Home Assistant automation blueprints
- Cover entity
  - `Lid` — open or close the fan lid passively when the fan is off
- Diagnostic sensors
  - WiFi signal dBm
  - WiFi signal percent
- Sub-device grouping — all entities appear as one logical device in Home Assistant
- Multi-fan support — run multiple `maxxair_fan:` instances with separate transmitter GPIOs

## Installation

### From GitHub

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/n8detar/esphome-maxxair-fan
    components: [ maxxair_fan ]
```

### From a local checkout

```yaml
external_components:
  - source:
      type: local
      path: /config/esphome/esphome-maxxair-fan/components
```

## Hardware

| Part | Notes |
| --- | --- |
| ESP8266 (Wemos D1 Mini) or ESP32 | Update pin assignments to match your board |
| IR LED (940 nm) | e.g. TSAL6100 or similar |
| 220 Ω resistor | Series current limiter for the LED |
| 5 V power supply | A 12 V → 5 V buck converter works well in vans/RVs |

```
3.3 V ──[220 Ω]──┤►├── GPIO (ir_led_pin)
```

## Single Fan Example

```yaml
esp8266:
  board: d1_mini

remote_transmitter:
  - id: ir_tx
    pin: D6
    carrier_duty_percent: 50%

maxxair_fan:
  - id: maxxair
    transmitter_id: ir_tx

fan:
  - platform: speed
    id: maxxair_fan_id
    name: Fan
    speed_count: 10
    output: fan_speed_output
    direction_output: fan_direction_output
    device_id: maxxair_device

switch:
  - platform: template
    id: ceiling_fan_mode
    name: Ceiling Fan Mode
    icon: mdi:ceiling-fan
    optimistic: true
    device_id: maxxair_device
    on_turn_on:
      - lambda: id(maxxair).send_ceiling(id(maxxair_fan_id).speed);
    on_turn_off:
      - lambda: id(maxxair).send_intake(id(maxxair_fan_id).speed);

  - platform: template
    id: auto_fan
    name: Auto Fan
    icon: mdi:fan-auto
    optimistic: true
    device_id: maxxair_device

cover:
  - platform: template
    id: maxxair_lid
    name: Lid
    optimistic: true
    device_id: maxxair_device
    on_open:
      - lambda: id(maxxair).send_lid_open();
    on_closed:
      - lambda: id(maxxair).send_lid_close();
```

See [`examples/maxxair-fan-example.yaml`](examples/maxxair-fan-example.yaml) for a complete configuration with the full fan state machine, sub-device grouping, WiFi diagnostics, secrets, OTA, and API.

## Two Fans from One ESP

```yaml
esphome:
  devices:
    - id: fan_front
      name: Front MaxxAir
    - id: fan_rear
      name: Rear MaxxAir

remote_transmitter:
  - id: ir_tx_front
    pin: D5
    carrier_duty_percent: 50%
  - id: ir_tx_rear
    pin: D6
    carrier_duty_percent: 50%

maxxair_fan:
  - id: maxxair_front
    transmitter_id: ir_tx_front
  - id: maxxair_rear
    transmitter_id: ir_tx_rear
```

Then reference `id(maxxair_front)` or `id(maxxair_rear)` in lambdas as needed.

## Component API

Available methods for use inside `lambda:` actions:

```cpp
// High-level helpers — speed is 1–10
id(maxxair).send_exhaust(speed);   // Air OUT, lid open
id(maxxair).send_intake(speed);    // Air IN, lid open
id(maxxair).send_ceiling(speed);   // Lid closed, air down, special flag set
id(maxxair).send_lid_open();       // Fan off, lid open (passive ventilation)
id(maxxair).send_lid_close();      // Fan off, lid closed

// Full control — speed_pct is 0–100
id(maxxair).send(fan_on, fan_exhaust, cover_open, speed_pct,
                 auto_mode, auto_temperature, special, warn);

// Example: thermostat auto-mode at 75 °F, intake, speed 50 %
id(maxxair).send(true, false, true, 50, true, 75);
```

## Auto Fan Blueprint

The [Smarty Van Auto Fan Blueprint](https://github.com/SmartyVan/MaxxAir-Fan-ESPHome/blob/main/smarty_van_autofan_blueprint.yaml) works directly with the `Auto Fan` switch. Import it into Home Assistant, select the switch, and configure temperature thresholds and speed range.

## Control Logic

| Action | Result |
| --- | --- |
| Fan FORWARD | Air OUT (exhaust), lid open |
| Fan REVERSE | Air IN (intake), lid open |
| Ceiling Fan Mode on | Forces REVERSE, closes lid, sets `special` flag |
| Ceiling Fan Mode off (fan on) | Opens lid, fan continues as REVERSE |
| Fan turns off | Clears Ceiling Fan Mode and Auto Fan |
| Lid OPEN (fan off) | Passive ventilation command |
| Lid CLOSE (fan on) | Engages Ceiling Fan Mode |

## Known Limitations

- IR is send-only. Physical button presses on the fan are not detected and will desync Home Assistant state.
- Auto mode temperature setpoint is sent with every command but the fan's built-in thermostat logic is separate from the `Auto Fan` switch, which is only a flag for use in HA automations.
- The `warn` (beep) flag is not exposed in the default example config.

## Credits

- IR protocol reverse-engineered by [skypeachblue](https://github.com/skypeachblue), [wingspinner](https://github.com/wingspinner), and [Jeff Brown](https://github.com/brown-studios/esphome-maxxfan-protocol)
- Fan state machine and entity structure based on work by [Mike Goubeaux @ Smarty Van](https://www.youtube.com/@SmartyVan)
