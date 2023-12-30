import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    CONF_ID,
)
from . import ECODAN, CONF_ECODAN_ID, ecodan_ns

AUTO_LOAD = ["ecodan"]

EcodanSwitch = ecodan_ns.class_("EcodanSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional("power_state"): switch.switch_schema(
            icon="mdi:lightning-bolt"
        ).extend(
            {
                cv.GenerateID(): cv.declare_id(EcodanSwitch),
            }
        ),
        cv.Optional("force_dhw"): switch.switch_schema(
            icon="mdi:water-plus"
        ).extend(
            {
                cv.GenerateID(): cv.declare_id(EcodanSwitch),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    heatpump = yield cg.get_variable(config[CONF_ECODAN_ID])

    switches = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        id = conf.get("id")
        if id and id.type == switch.Switch:
            var = cg.new_Pvariable(conf[CONF_ID])
            yield switch.register_switch(var, conf)
            cg.add(getattr(heatpump, f"set_{key}")(var))
            cg.add(var.set_key(key))
            switches.append(f"F({key})")

    cg.add_define(
        "ECODAN_SWITCH_LIST(F, sep)", cg.RawExpression(" sep ".join(switches))
    )