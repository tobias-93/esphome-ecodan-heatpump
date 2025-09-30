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
CONF_MODE_SELECT_Z1 = "mode_select_zone1"
CONF_MODE_SELECT_Z2 = "mode_select_zone2"
CONF_HOT_WATER_MODE = "hot_water_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional(CONF_MODE_SELECT): select.select_schema(
            EcodanModeSelect,
            icon="mdi:sun-snowflake-variant",
        ),
        cv.Optional(CONF_MODE_SELECT_Z1): select.select_schema(
            EcodanModeSelect,
            icon="mdi:sun-snowflake-variant",
        ),
        cv.Optional(CONF_MODE_SELECT_Z2): select.select_schema(
            EcodanModeSelect,
            icon="mdi:sun-snowflake-variant",
        ),
        cv.Optional(CONF_HOT_WATER_MODE): select.select_schema(
            EcodanModeSelect,
            icon="mdi:hand-water",
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    heatpump = await cg.get_variable(config[CONF_ECODAN_ID])

    selects = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == select.Select:
            if key == CONF_MODE_SELECT or key == CONF_MODE_SELECT_Z1 or key == CONF_MODE_SELECT_Z2:
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
            var = await select.new_select(conf, options=options)
            # map legacy key to zone1 setter by default
            setter_key = key
            if key == CONF_MODE_SELECT:
                setter_key = CONF_MODE_SELECT_Z1
            cg.add(getattr(heatpump, f"set_{setter_key}")(var))
            # Keep the entity key as the user provided key (for entity naming and config)
            cg.add(var.set_key(key))
            # The ECODAN_SELECT_LIST macro must reference the internal compile-time name
            internal_key = key
            if key == CONF_MODE_SELECT:
                internal_key = CONF_MODE_SELECT_Z1
            selects.append(f"F({internal_key})")

    cg.add_define(
        "ECODAN_SELECT_LIST(F, sep)", cg.RawExpression(" sep ".join(selects))
    )

