# ESPHome Ecodan Heatpump

Read and control Mitsubishi Ecodan air-water heat pumps from Home Assistant over the CN105 connector.

> **⚠️ Active development has moved**
> The `main` branch is kept around for compatibility. New work happens on [`future-climate-2zone`](https://github.com/tobias-93/esphome-ecodan-heatpump/tree/future-climate-2zone). For new installs, use that branch — once feedback has been processed it will become the new `main` and this branch will be deprecated.

## Compatibility

Confirmed working and tested with:
- ERST20D-VM2D
- EHSD20D-YM9D

Likely works with most Mitsubishi air-water heat pumps that expose a CN105 connector.

## Credits

Inspired by [BartGijsbers/CN105Gateway](https://github.com/BartGijsbers/CN105Gateway). Hardware reference: [SwiCago/HeatPump](https://github.com/SwiCago/HeatPump).

---

## Table of contents

- [Hardware](#hardware)
- [Installation — experienced](#installation--experienced-esphome-users)
- [Installation — step by step](#installation--step-by-step-new-to-esphome)
- [Wiring to the heat pump](#wiring-to-the-heat-pump)
- [Cookbook](#cookbook)
- [Contributing](#contributing)
- [Help](#help)

---

## Hardware

### Bill of materials

| Part | Notes | Link |
|------|-------|------|
| CN105 cable | **JST PA 2.0mm** to Dupont female, 5-pin (`PAP-05V-S`) | [AliExpress](https://aliexpress.com/item/1005005562174022.html) |
| ESP-01S | Use the **ESP-01S** — it has more memory than the plain ESP-01 | [AliExpress](https://www.aliexpress.com/item/32582736130.html) |
| Power breakout (converts 5V to 3.3V) | | [AliExpress](https://aliexpress.com/item/1005006492591912.html) |
| USB-to-serial adapter | Only needed for the initial flash if you don't already own one | [AliExpress](https://aliexpress.com/item/1005010685046335.html) |

### Connector

The CN105 is a **5-pin JST PA 2.0mm** connector.

> ⚠️ Do **not** confuse this with the JST **PH** 2.0mm series — the pitch is identical but the housing and locking mechanism are different. A PH connector will not fit the CN105 port.

#### Pinout

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | +12V | Power — **do not use**, unstable under compressor load |
| 2 | GND | Ground |
| 3 | +5V | Power supply for the ESP |
| 4 | TX | Data from heat pump → connect to ESP **RX** |
| 5 | RX | Data to heat pump ← connect to ESP **TX** |

> 📌 Pin 1 and pin 5 are usually marked near the CN105 port on the PCB.

#### Wire colours (PAP-05V-S cable)

The `PAP-05V-S` pigtail cable has the following wire colours:

| Pin | Colour | Signal |
|-----|--------|--------|
| 1 | Black | +12V — **cut this wire off completely** |
| 2 | Red | GND |
| 3 | White | +5V |
| 4 | Yellow | TX (pump) |
| 5 | Orange | RX (pump) |

> ⚠️ **Cut the black wire (pin 1) off completely.** It carries 12V and is not needed to power the ESP. Leaving it connected risks damaging the ESP if it accidentally contacts another pin.

---

## Installation — experienced ESPHome users

1. Create a new **ESP8266** device in the ESPHome dashboard.
2. Add to your `secrets.yaml`:
   - `wifi_ssid`
   - `wifi_password`
   - `heatpump_ota_password`
   - `heatpump_encryption_key`
3. Replace the generated device YAML with [`examples/heatpump.yaml`](./examples/heatpump.yaml).
4. Flash via USB using [ESPHome Web](https://web.esphome.io/?dashboard_install).
5. Connect to the heat pump's CN105 port (see [Wiring](#wiring-to-the-heat-pump)). Home Assistant auto-discovers the device.

---

## Installation — step by step (new to ESPHome)

### 1. Install ESPHome

Install the ESPHome add-on in Home Assistant — see the [official getting-started guide](https://esphome.io/guides/getting_started_hassio).

### 2. Create the device

1. Open **ESPHome** from **Settings → Add-ons** and click **Open Web UI**.
2. Click **New Device** (bottom-right).
3. Name it (e.g. *Ecodan Heatpump*) and choose **Skip this step**.
4. Select **ESP8266**.
5. **Copy the encryption key** to a scratchpad — it looks like `pgdlhjfgkasdhfgeury3874iuygjg748gjhgfds32=`.
6. Click **Skip**.

### 3. Configure the YAML

1. The new device shows as **OFFLINE**. Click **Edit**.
2. **Copy the OTA password** to your scratchpad — it looks like `"a248d5bc6dae01010101670250c1aadadac1"`.
3. Replace the entire template YAML with the contents of [`examples/heatpump.yaml`](./examples/heatpump.yaml).
4. **Save** and close the file with the **×** next to the filename.

### 4. Fill in the secrets file

Click **Secrets** (top-right of the ESPHome Web UI) and add:

```yaml
wifi_ssid: "MySSID"
wifi_password: "MyWiFiPassword"
heatpump_ota_password: "a248d5bc6dae01010101670250c1aadadac1"
heatpump_encryption_key: "pgdlhjfgkasdhfgeury3874iuygjg748gjhgfds32="
```

Save and close.

### 5. Flash the ESP

1. Click the **three dots** on the device card → **Install**.

   <img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/a7a16433-3b1f-4bab-9eac-08de005f97e6" width="600">

2. Select **Plug into this computer** and open [ESPHome Web](https://web.esphome.io/?dashboard_install).

   > **Note:** Open the browser via the **local IP** of your Home Assistant server. If you reach HA through nginx, Apache or a tunnel, the browser cannot access the COM port.

3. With the ESP plugged into your computer's USB port, click **Connect**. If no ports show up, install the [FTDI driver](https://ftdichip.com/drivers/).
4. Select the port → **Connect** → **Prepare for first time use**.

   <img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/ed8d7561-4154-4607-a284-4b8fd4cbefe4" width="400">

5. When the install finishes, click **Close** and power-cycle the ESP.

### 6. Add to Home Assistant

Back in the ESPHome Web UI the device should now show **ONLINE**. Home Assistant auto-discovers it under **Settings → Devices & Services**.

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/123979cb-41b5-4da6-94c9-999cc6dd9497" width="300">

Configure and add the node — it appears as a regular device. Continue with [Wiring](#wiring-to-the-heat-pump) so the sensors get values.

---

## Wiring to the heat pump

1. Remove the black wire (pin 1) from the connector completely — it carries 12V and is not needed.
2. If a Wi-Fi dongle is already plugged into the CN105 port, remove it first. Only one device can be connected at a time.
3. Locate the CN105 port on your heat pump PCB — it is labelled **CN105** on the board:

   ![CN105 port on ERST20D-VM2D](https://raw.githubusercontent.com/joohann/esphome-ecodan-heatpump/main/image.jpeg)

4. Connect ESP ↔ CN105 as follows:

   | CN105 Pin | Wire colour (PAP-05V-S) | ESP-01S Pin |
   |-----------|------------------------|-------------|
   | 2 | Red | GND |
   | 3 | White | VCC (3.3V via power breakout) |
   | 4 | Yellow | RX |
   | 5 | Orange | TX |

   <table><tr>
   <td><img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/7fe0ee6e-9020-47ed-b0d4-abb33d688eab" width="400"></td>
   <td><img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/784ab6b1-c9bc-4738-ba68-6a278cef2244" width="400"></td>
   </tr></table>

5. Within a minute or two, all sensors of your Ecodan device populate in Home Assistant:

   <img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/f15695cd-aaca-4c34-b852-bd0ec2742110" width="800">

---

## Cookbook

### Set room temperature from a remote sensor

> **Note:** this may not work when an external Mitsubishi control panel is connected — it appears to only work without one.

The heat pump can be told the room temperature instead of relying on its internal sensor. Call `setRemoteTemperature(float temp)` on the `ecodan` object inside a lambda. Pass `0` to fall back to the internal sensor.

There are two common patterns.

#### Option A — push from a sensor

```yaml
ecodan:
  id: ecodan_instance
  uart_id: ecodan_uart

sensor:
  # Bluetooth temperature sensor
  - platform: atc_mithermometer
    mac_address: "XX:XX:XX:XX:XX:XX"
    temperature:
      name: "Lounge temperature"
      on_value:
        then:
          - lambda: 'id(ecodan_instance).set_remote_temperature(x);'

  # ...or a Home Assistant sensor
  - platform: homeassistant
    name: "Temperature Sensor From Home Assistant"
    entity_id: sensor.temperature_sensor
    on_value:
      then:
        - lambda: 'id(ecodan_instance).set_remote_temperature(x);'
```

#### Option B — expose services to Home Assistant

```yaml
api:
  services:
    - service: set_remote_temperature
      variables:
        temperature: float
      then:
        - lambda: 'id(ecodan_instance).set_remote_temperature(temperature);'

    - service: use_internal_temperature
      then:
        - lambda: 'id(ecodan_instance).set_remote_temperature(0);'
```

Inspired by [geoffdavis/esphome-mitsubishiheatpump](https://github.com/geoffdavis/esphome-mitsubishiheatpump#remote-temperature).

---

## Contributing

Open an issue or PR if something is missing or could be improved.

## Help

Join the discussion on [Gitter](https://app.gitter.im/#/room/#Mitsubishi-CN105-Protocol-Decode_community:gitter.im).
