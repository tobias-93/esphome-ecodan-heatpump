import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    CONF_ID,
    UNIT_CELSIUS,
)
from . import ECODAN, CONF_ECODAN_ID, ecodan_ns

AUTO_LOAD = ["ecodan"]

EcodanNumber = ecodan_ns.class_("EcodanNumber", number.Number, cg.Component)

CONF_HOT_WATER_SETPOINT = "hot_water_setpoint"
CONF_ZONE1_ROOM_TEMP_SETPOINT = "zone1_room_temp_setpoint"
CONF_ZONE1_FLOW_TEMP_SETPOINT = "zone1_flow_temp_setpoint"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional(CONF_HOT_WATER_SETPOINT): number.number_schema(
            class_=EcodanNumber,
            icon="mdi:hand-water",
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
        ),
        cv.Optional(CONF_ZONE1_ROOM_TEMP_SETPOINT): number.number_schema(
            class_=EcodanNumber,
            icon="mdi:home-thermometer",
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
        ),
        cv.Optional(CONF_ZONE1_FLOW_TEMP_SETPOINT): number.number_schema(
            class_=EcodanNumber,
            icon="mdi:water-thermometer",
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    heatpump = yield cg.get_variable(config[CONF_ECODAN_ID])

    numbers = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == number.Number:
            if key == CONF_HOT_WATER_SETPOINT:
                min_value = 40
                max_value = 60
                step = 1
            elif key == CONF_ZONE1_ROOM_TEMP_SETPOINT:
                min_value = 10
                max_value = 30
                step = 0.5
            elif key == CONF_ZONE1_FLOW_TEMP_SETPOINT:
                min_value = 20
                max_value = 60
                step = 0.5
            else:
                min_value = 0
                max_value = 100
                step = 1
            var = cg.new_Pvariable(conf[CONF_ID])
            yield number.register_number(var, conf, min_value=min_value, max_value=max_value, step=step)
            cg.add(getattr(heatpump, f"set_{key}")(var))
            cg.add(var.set_key(key))
            numbers.append(f"F({key})")

    cg.add_define(
        "ECODAN_NUMBER_LIST(F, sep)", cg.RawExpression(" sep ".join(numbers))
    )