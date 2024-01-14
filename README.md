# ESPHome components for Ecodan heatpumps
This is a set of components to read out and control Mitsubishi Ecodan heatpumps. I have an ERST20D-VM2D, it probably works for many air-water heatpumps with CN105 connector. It is highly inspired by https://github.com/BartGijsbers/CN105Gateway.

## Hardware
Info about the hardware can be found at https://github.com/SwiCago/HeatPump. I used the following:
- https://www.aliexpress.com/item/1005003547145418.html (take the PH2.0 to Dupont, 5P variant of the connector, it fits by cutting away some plastic)
- https://www.aliexpress.com/item/32582736130.html (take the ESP-01S, it has some more memory)
- https://www.aliexpress.com/item/4001165244572.html

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