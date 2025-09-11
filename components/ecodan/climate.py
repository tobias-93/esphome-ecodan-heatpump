import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import (
    CONF_ID,
)
from . import ECODAN, CONF_ECODAN_ID, ecodan_ns

AUTO_LOAD = ["ecodan"]

EcodanClimate = ecodan_ns.class_("EcodanClimate", climate.Climate, cg.Component)

CONF_ZONE1 = "zone1"
CONF_ZONE2 = "zone2"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional(CONF_ZONE1): climate.climate_schema(
            class_=EcodanClimate,
        ),
        cv.Optional(CONF_ZONE2): climate.climate_schema(
            class_=EcodanClimate,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    heatpump = await cg.get_variable(config[CONF_ECODAN_ID])

    climates = []
    for zone_key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == climate.Climate:
            var = await climate.new_climate(conf)
            
            if zone_key == CONF_ZONE1:
                cg.add(var.set_zone(1))
                zone_name = "zone1"
            elif zone_key == CONF_ZONE2:
                cg.add(var.set_zone(2))
                zone_name = "zone2"
            else:
                continue
                
            cg.add(var.set_temperature_range(10.0, 30.0))
            cg.add(var.set_temperature_step(0.5))
            
            cg.add(getattr(heatpump, f"set_climate_{zone_name}")(var))
            cg.add(var.set_heatpump(heatpump))
            
            climates.append(f"F({zone_name})")

    if climates:
        cg.add_define(
            "ECODAN_CLIMATE_LIST(F, sep)", cg.RawExpression(" sep ".join(climates))
        )
