import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_WATER,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL,
    UNIT_CELSIUS,
    UNIT_HERTZ,
    UNIT_HOUR,
    UNIT_KILOWATT,
    UNIT_KILOWATT_HOURS,
)
from . import ECODAN, CONF_ECODAN_ID

AUTO_LOAD = ["ecodan"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional("error_code"): sensor.sensor_schema(
            icon="mdi:alert",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional("frequency"): sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            icon="mdi:sine-wave",
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("output_power"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT,
            icon="mdi:home-lightning-bolt",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("zone1_room_temperature"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:home-thermometer",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("gas_return_temp_signed"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-lines",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("outside_temperature"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:sun-thermometer",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("water_feed_temp_signed"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:coolant-temperature",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("water_return_temp_signed"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:coolant-temperature",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("hot_water_temperature"): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:hand-water",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("runtime"): sensor.sensor_schema(
            unit_of_measurement=UNIT_HOUR,
            icon="mdi:clock",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional("water_flow"): sensor.sensor_schema(
            unit_of_measurement="l/m",
            icon="mdi:waves-arrow-right",
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("energy_cons_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-export",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
        cv.Optional("energy_prod_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-import",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
        cv.Optional("energy_cooling_cons_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-export",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
        cv.Optional("energy_cooling_prod_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-import",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
        cv.Optional("energy_dhw_cons_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-export",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
        cv.Optional("energy_dhw_prod_yesterday"): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            icon="mdi:transmission-tower-import",
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    heatpump = yield cg.get_variable(config[CONF_ECODAN_ID])

    sensors = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == sensor.Sensor:
            s = yield sensor.new_sensor(conf)
            cg.add(getattr(heatpump, f"set_{key}")(s))
            sensors.append(f"F({key})")

    cg.add_define("ECODAN_SENSOR_LIST(F, sep)", cg.RawExpression(" sep ".join(sensors)))
