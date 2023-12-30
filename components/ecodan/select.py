import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    CONF_ID,
    CONF_OPTIONS,
)
from . import ECODAN, CONF_ECODAN_ID, ecodan_ns

AUTO_LOAD = ["ecodan"]

EcodanModeSelect = ecodan_ns.class_("EcodanSelect", select.Select, cg.Component)

CONF_MODE_SELECT = "mode_select"
CONF_HOT_WATER_MODE = "hot_water_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional(CONF_MODE_SELECT): select.select_schema(
            icon="mdi:sun-snowflake-variant",
        ).extend(
            {
                cv.GenerateID(): cv.declare_id(EcodanModeSelect),
            }
        ),
        cv.Optional(CONF_HOT_WATER_MODE): select.select_schema(
            icon="mdi:hand-water",
        ).extend(
            {
                cv.GenerateID(): cv.declare_id(EcodanModeSelect),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    heatpump = yield cg.get_variable(config[CONF_ECODAN_ID])

    selects = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == select.Select:
            if key == CONF_MODE_SELECT:
                options = [
                    "Heating Room Temp",
                    "Heating Flow Temp",
                    "Heating Heat Curve",
                    "Cooling Room Temp",
                    "Cooling Flow Temp",
                ]
            elif key == CONF_HOT_WATER_MODE:
                options = [
                    "Normal",
                    "Economy",
                ]
            else:
                options = []
            var = cg.new_Pvariable(conf[CONF_ID])
            yield select.register_select(var, conf, options=options)
            cg.add(getattr(heatpump, f"set_{key}")(var))
            cg.add(var.set_key(key))
            selects.append(f"F({key})")

    cg.add_define(
        "ECODAN_SELECT_LIST(F, sep)", cg.RawExpression(" sep ".join(selects))
    )

