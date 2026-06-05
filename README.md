# ESPHome Velit

Unofficial ESPHome external component for controlling Velit air conditioners and heaters over BLE. The component exposes supported Velit devices through standard ESPHome entities, including climate controls, diagnostics, and helper controls for features that do not map cleanly to ESPHome climate traits.

This repository is designed to be used directly as an ESPHome `external_components` source from GitHub or from a local checkout.

## Important Notes

- This is an unofficial, reverse-engineered project and is not affiliated with or supported by Velit.
- This project was vibe-coded, is provided as-is, and should be used at your own risk.
- BLE access is exclusive. While ESPHome is connected to the heater or AC, the Velit Android app cannot connect. If the Android app is connected, ESPHome cannot connect.
- The examples include a `ble_client` switch so ESPHome can be intentionally disconnected before using the Android app, then reconnected afterward.
- OTA and Wi-Fi provisioning are out of scope for this project and will not be added.

## Requirements

- ESP32-based ESPHome node with BLE enabled.
- Fixed Velit BLE MAC address configured through ESPHome `ble_client`.
- One Velit device per `ble_client`.

## Installation

### GitHub

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/n8detar/esphome-velit
    components: [velit]
```

### Local Checkout

```yaml
external_components:
  - source:
      type: local
      path: /config/esphome/esphome-velit/components
    components: [velit]
```

## Supported Entities

| Device | Entity | Notes |
| --- | --- | --- |
| AC | `climate` | Supports `OFF`, `HEAT`, `COOL`, `FAN_ONLY`, target temperature, current temperature, and custom fan modes `Speed 1` through `Speed 5`. |
| AC | `switch` | `swing` toggle. |
| AC | `sensor` | Ambient temperature and fault code. |
| AC | `text_sensor` | Fault text. AC fault text is currently the raw fault code string. |
| Heater | `climate` | Supports `OFF`, `HEAT`, target temperature, and current temperature. |
| Heater | `select` | `operating_mode` with `Thermostat` and `Manual`. |
| Heater | `number` | `manual_fan_speed` from `1` to `5`. This is only meaningful when the heater is in manual mode. |
| Heater | `sensor` | Ambient temperature, fault code, and altitude. Altitude is reported by the heater in feet and marked with the Home Assistant distance device class. |
| Heater | `text_sensor` | Fault text using the app's known heater fault mapping. |

## Home Assistant Device Grouping

The examples use ESPHome sub-devices so the Velit heater or AC appears as a separate Home Assistant device from the ESP controller. Define the Velit device under `esphome.devices`, then set `device_id` on each Velit-related entity. Remove the `device_id` entries if you prefer all entities to stay under the ESP node.

## AC Example

```yaml
esphome:
  name: velit-ac-example
  devices:
    - id: velit_ac_device
      name: Velit AC

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
    device_id: velit_ac_device

sensor:
  - platform: velit
    velit_id: velit_ac
    ambient_temperature:
      name: Velit AC Ambient Temperature
      device_id: velit_ac_device
    fault_code:
      name: Velit AC Fault Code
      device_id: velit_ac_device

text_sensor:
  - platform: velit
    velit_id: velit_ac
    fault_text:
      name: Velit AC Fault Text
      device_id: velit_ac_device

switch:
  - platform: ble_client
    ble_client_id: velit_ac_ble
    name: Velit AC BLE Connection
    device_id: velit_ac_device
  - platform: velit
    velit_id: velit_ac
    swing:
      name: Velit AC Swing
      device_id: velit_ac_device
```

## Heater Example

```yaml
esphome:
  name: velit-heater-example
  devices:
    - id: velit_heater_device
      name: Velit Heater

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
    device_id: velit_heater_device

sensor:
  - platform: velit
    velit_id: velit_heater
    ambient_temperature:
      name: Velit Heater Ambient Temperature
      device_id: velit_heater_device
    fault_code:
      name: Velit Heater Fault Code
      device_id: velit_heater_device
    altitude:
      name: Velit Heater Altitude
      device_id: velit_heater_device

text_sensor:
  - platform: velit
    velit_id: velit_heater
    fault_text:
      name: Velit Heater Fault Text
      device_id: velit_heater_device

switch:
  - platform: ble_client
    ble_client_id: velit_heater_ble
    name: Velit Heater BLE Connection
    device_id: velit_heater_device

select:
  - platform: velit
    velit_id: velit_heater
    operating_mode:
      name: Velit Heater Operating Mode
      device_id: velit_heater_device

number:
  - platform: velit
    velit_id: velit_heater
    manual_fan_speed:
      name: Velit Heater Manual Fan Speed
      device_id: velit_heater_device
```

Full local validation examples are available in [`examples/ac.yaml`](examples/ac.yaml) and [`examples/heater.yaml`](examples/heater.yaml).

## Behavior

- Temperatures are normalized to Celsius internally.
- Fahrenheit-range replies from the heater are converted to Celsius before being published.
- Target temperature writes are sent in the Celsius-style range used by the Velit BLE protocol.
- AC polling refreshes power, mode, target temperature, fan speed, ambient temperature, swing, and fault status.
- Heater polling refreshes operating mode, fan speed, target temperature, ambient temperature, altitude, power state, and fault status.
- On reconnect, the component re-discovers BLE characteristics, re-subscribes to notifications, and resumes polling.

## Logging

Normal builds log component configuration and actionable BLE connection warnings. Raw BLE packet TX/RX logging is intentionally disabled for release use so routine ESPHome logs stay readable.

When BLE scans see likely Velit advertisements, the component logs the candidate MAC address once per boot. Candidates are matched by the known `FFE0` Velit BLE service UUID or by app-derived advertised-name markers such as `VELIT`, `VLIT`, and `D30`. This is only a helper for finding the MAC address; binding is still fixed through `ble_client.mac_address`.

If you do not know the MAC address yet, use a temporary placeholder such as `00:11:22:33:44:55`, install the example, watch the ESPHome logs for the `INFO` line `Discovered possible Velit BLE device ...`, then replace the placeholder with the logged MAC.

## Known Limitations

- ESP32 only.
- Fixed BLE MAC binding only.
- AC vendor submodes such as `sleep`, `eco`, `auto`, `turbo`, and `vent` are not currently exposed.
- Heater timer programming and dashboard diagnostics are not currently exposed.
- Heater clock sync is best effort and only works when the ESPHome node already has valid system time.
- OTA and Wi-Fi provisioning will not be added.

## To Do

- Future release: if the protocol mapping is reliable enough, expose AC vendor submodes such as `sleep`, `eco`, `auto`, `turbo`, and `vent` as climate presets.
