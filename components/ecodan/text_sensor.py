import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import ECODAN, CONF_ECODAN_ID

AUTO_LOAD = ["ecodan"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional("date_time"): text_sensor.text_sensor_schema(
            text_sensor.TextSensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional("defrost"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("heating_stage"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("operating_mode"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("heat_cool"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("holiday_mode"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("hot_water_timer"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("date_energy_cons"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
        cv.Optional("date_energy_prod"): text_sensor.text_sensor_schema(text_sensor.TextSensor),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    heatpump = await cg.get_variable(config[CONF_ECODAN_ID])

    text_sensors = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == text_sensor.TextSensor:
            var = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(heatpump, f"set_{key}")(var))
            text_sensors.append(f"F({key})")

    cg.add_define(
        "ECODAN_TEXT_SENSOR_LIST(F, sep)", cg.RawExpression(" sep ".join(text_sensors))
    )