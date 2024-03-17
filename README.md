# ESPHome components for Ecodan heatpumps
This is a set of components to read out and control Mitsubishi Ecodan heatpumps. I have an ERST20D-VM2D and it is also confirmed to work with EHSD20D-YM9D. It probably works for many air-water heatpumps with CN105 connector. 

It is highly inspired by https://github.com/BartGijsbers/CN105Gateway.

## Table of Contents
- [Hardware](#hardware)
- [Installing](#installing)
  - [If you are experienced with ESPHome](#if-you-are-experienced-with-esphome)
  - [If you are new to ESPHome](#if-you-are-new-to-esphome)
- [Cookbook](#cookbook)
- [Contributing](#contributing)
- [Help](#help)

___

## Hardware
Info about the hardware can be found at https://github.com/SwiCago/HeatPump. I used the following:
- https://www.aliexpress.com/item/1005003547145418.html (take the PH2.0 to Dupont, 5P variant of the connector, it fits by cutting away some plastic)
- https://www.aliexpress.com/item/32582736130.html (take the ESP-01S, it has some more memory)
- https://www.aliexpress.com/item/4001165244572.html
- if you don't already have an adapter to connect it to your PC for initial programming: https://www.aliexpress.com/item/32688280601.html

## Installing
### If you are experienced with ESPHome
1. Create a new ESP8266 device in the ESPHome web UI
2. Update your **Secrets file** with *wifi_ssid*, *wifi_password*, *heatpump_ota_password* and *heatpump_encryption_key*
3. Edit your device yaml-code and replace the template code with the contents in [heatpump.yaml](./examples/heatpump.yaml)
4. Attach your ESP8266 to the USB-adapter and connect to your PC. Open [ESPHome Web](https://web.esphome.io/?dashboard_install) and install the ESP-device
5. Connect the ESP to your Ecodan CN105-port and it will be auto-detected by Home Assistant. Add the device and all the sensors will appear with updated values.

### If you are new to ESPHome
1. Install ESPHome in HomeAssistant by following [this instruction](https://esphome.io/guides/getting_started_hassio)
2. Open **ESPHome** from **Settings / Add-ons**. Klick **OPEN WEB UI**
* Click **NEW DEVICE** in the bottom right corner
* Give your device a new name, for example **Ecodan Heatpump**, and select **SKIP THIS STEP**
* Select **ESP8266** and your device will be created
* Copy the Encryption key to a scratch-pad so you can access it later. It looks something like this *pgdlhjfgkasdhfgeury3874iuygjg748gjhgfds32=*
* Click **SKIP**
2. Now your screen should have a device with the name you selected and state **OFFLINE**
* Click **Edit* to open the device specific yaml-file
* Copy the OTA: password to the scratch-pad you used for the encryption key. It looks something like this: *"a248d5bc6dae01010101670250c1aadadac1"*
3. Replace all of the template *yaml* code with the contents in [heatpump.yaml](./examples/heatpump.yaml) located in the examples folder of this repository
* Click **SAVE** in the upper right corner and close the file with the **X** on the left side, next to the filename.
4. Open the secrets file by clicking **SECRETS** in the upper right corner of the ESPHome Web UI. Add your Wi-Fi credentials along with the OTA password and the encryption key. It should look something like this;

```yaml
# Your Wi-Fi SSID and password
wifi_ssid: "MySSID"
wifi_password: "MyWiFiPassword"
heatpump_ota_password: "a248d5bc6dae01010101670250c1aadadac1"
heatpump_encryption_key: "pgdlhjfgkasdhfgeury3874iuygjg748gjhgfds32="
```
* Save and close the file.
5. Back in the ESPHome Web UI, click the **three dots** in the bottom right corner of your device and select **Install**

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/a7a16433-3b1f-4bab-9eac-08de005f97e6" width="600">

If you have an USB-to-ESP device, it should be possible to install directly from ESPHome. 
###### Note: make sure that you connect your web-browser to the local IP of the HomeAssistant server. If you connect via internet, for example connect via nginx or Apache, it will not be possible to connect to the COM-port of your computer.

* Select **Plug into this computer** and in the next step, Open [**ESPHome Web**](https://web.esphome.io/?dashboard_install)
  
   Make sure your ESP is connected via USB to the computer where you opened the browser (it does not have to be on the HomeAssistant server) and click **CONNECT**
  
   You should see a list of COM ports (If you do not, you probably need to install an updated [FTDI driver](https://ftdichip.com/drivers/))
* Select the port with the ESP connected and click **Connect**
  
   In the next step you should see an option like below, Select **Prepare for first time use**

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/ed8d7561-4154-4607-a284-4b8fd4cbefe4" width="400">


6. Once installed, click **Close**
7. Power-cycle your ESP and return to the **ESPHome Web UI**, your Heatpump device should now say **ONLINE** in the upper right corner.
  
   It is auto-detected by HomeAssistant so just open **Settings / Devices & Services** and it will appear

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/123979cb-41b5-4da6-94c9-999cc6dd9497" width="300">

7. Configure and add the ESPHome node to Home Assistant and it will appear as a Device.

8. Now we need to connect it to the Ecodan Heatpump for the values will populate
   
   Cutting the cable to fit takes a few attempts. Luckily, the link above gets you 10 cables so don't worry if you happen to break one!

   I recommend to cut the black cable off completely so you do not accidentally connect 12V to the ESP

   The cable should look something like this when trimmed dow to fit the socket for the CN105 port (if you have a Wi-Fi dongle connected you may need to remove that from the port and connect the ESP-device in its place)

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/3c37f017-533e-4338-9c83-4fb7df96d063" width="400">

9. Connect it to the ESP and CN105 port of your Ecodan-system as illustrated in the pictures below.

<table><tr><td>
<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/7fe0ee6e-9020-47ed-b0d4-abb33d688eab" width="400"></td>
<td><img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/784ab6b1-c9bc-4738-ba68-6a278cef2244" width="400">
</td></tr></table>


10. Within a minute or two, numbers will populate on all the sensors generated by your Ecodan Heater device in Home Assistant

<img src="https://github.com/hallonstedt/esphome-ecodan-heatpump/assets/55149768/f15695cd-aaca-4c34-b852-bd0ec2742110" width="800">

## Cookbook
### Set room temperature from a remote sensor
*Note that this didn't work for me, I have an external Mitsubishi control panel connected. Maybe it only works when there's no external panel connected.*

It is possible to use an external temperature sensor to tell the heat pump what the room temperature is, rather than relying on its internal temperature sensor. You can do this by calling `setRemoteTemperature(float temp)` on the `ecodan` object in a lambda. Note that you can call `setRemoteTemperature(0)` to switch back to the internal temperature sensor.

There are several ways you could make use of this functionality. One is to use a sensor automation:

```yaml
ecodan:
  id: ecodan_instance
  uart_id: ecodan_uart

sensor:
  # You could use a Bluetooth temperature sensor
  - platform: atc_mithermometer
    mac_address: "XX:XX:XX:XX:XX:XX"
    temperature:
      name: "Lounge temperature"
      on_value:
        then:
          - lambda: 'id(ecodan_instance).set_remote_temperature(x);'

  # Or you could use a HomeAssistant sensor
  - platform: homeassistant
    name: "Temperature Sensor From Home Assistant"
    entity_id: sensor.temperature_sensor
    on_value:
      then:
        - lambda: 'id(ecodan_instance).set_remote_temperature(x);'
```

Alternatively you could define a [service](https://www.esphome.io/components/api.html#user-defined-services) that HomeAssistant can call:

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

Inspired by https://github.com/geoffdavis/esphome-mitsubishiheatpump#remote-temperature.

## Contributing
Let me know if there is anything you are missing or if you have improvement ideas.

## Help
Join the discussions on [Gitter](https://app.gitter.im/#/room/#Mitsubishi-CN105-Protocol-Decode_community:gitter.im)

