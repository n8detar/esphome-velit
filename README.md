# Velit ESPHome External Component

ESPHome external component for controlling Velit air conditioners and heaters over BLE using the same protocol found in Velit's Android app. This was vibe coded and exists as is. Use at your own risk.

## Important Notes

- BLE access is effectively exclusive. While ESPHome is connected to the heater or AC, the Velit Android app will not be able to connect. The reverse is also true: if the Android app is connected, ESPHome will not be able to connect.
- The examples below include a `ble_client` switch so you can disconnect ESPHome from the device before using the Android app, then reconnect it afterward.

## Features

- Shared `velit:` BLE hub bound to a `ble_client`
- Air conditioner climate entity
  - `OFF`, `HEAT`, `COOL`, `FAN_ONLY`
  - target temperature
  - custom fan modes `Speed 1` through `Speed 5`
  - swing switch
- Heater climate entity
  - `OFF`, `HEAT`
  - target temperature
  - separate `operating_mode` select
  - separate `manual_fan_speed` number
- Diagnostic entities
  - `fault_code`
  - `fault_text`
  - heater `altitude`

## Installation

### From GitHub

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/n8detar/esphome-velit
```

### From a local checkout

```yaml
external_components:
  - source:
      type: local
      path: /config/esphome/esphome-velit/components
```

## AC Example

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: 00:11:22:33:44:55
    id: velit_ac_ble

velit:
  - id: velit_ac
    ble_client_id: velit_ac_ble
    device_type: ac

climate:
  - platform: velit
    velit_id: velit_ac
    name: Velit AC

sensor:
  - platform: velit
    velit_id: velit_ac
    ambient_temperature:
      name: Velit AC Ambient Temperature
    fault_code:
      name: Velit AC Fault Code

text_sensor:
  - platform: velit
    velit_id: velit_ac
    fault_text:
      name: Velit AC Fault Text

switch:
  - platform: ble_client
    ble_client_id: velit_ac_ble
    name: Velit AC BLE Connection
  - platform: velit
    velit_id: velit_ac
    swing:
      name: Velit AC Swing
```

## Heater Example

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: 00:11:22:33:44:55
    id: velit_heater_ble

velit:
  - id: velit_heater
    ble_client_id: velit_heater_ble
    device_type: heater

climate:
  - platform: velit
    velit_id: velit_heater
    name: Velit Heater

sensor:
  - platform: velit
    velit_id: velit_heater
    ambient_temperature:
      name: Velit Heater Ambient Temperature
    fault_code:
      name: Velit Heater Fault Code
    altitude:
      name: Velit Heater Altitude

text_sensor:
  - platform: velit
    velit_id: velit_heater
    fault_text:
      name: Velit Heater Fault Text

switch:
  - platform: ble_client
    ble_client_id: velit_heater_ble
    name: Velit Heater BLE Connection

select:
  - platform: velit
    velit_id: velit_heater
    operating_mode:
      name: Velit Heater Operating Mode

number:
  - platform: velit
    velit_id: velit_heater
    manual_fan_speed:
      name: Velit Heater Manual Fan Speed
```

## Known Limitations

- ESP32 only
- Fixed BLE MAC binding only
- AC vendor submodes such as `sleep`, `eco`, `auto`, `turbo`, and `vent` are not exposed in v1
- Heater timer programming, dashboard diagnostics, OTA, and Wi-Fi provisioning are not exposed in v1
- Heater clock sync is best effort and only works when the ESPHome node already has valid system time
