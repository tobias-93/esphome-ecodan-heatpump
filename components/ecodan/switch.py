import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID
from . import ECODAN, CONF_ECODAN_ID, ecodan_ns

AUTO_LOAD = ["ecodan"]

EcodanSwitch = ecodan_ns.class_("EcodanSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ECODAN_ID): cv.use_id(ECODAN),
        cv.Optional("power_state"): switch.switch_schema(EcodanSwitch, icon="mdi:lightning-bolt"),
        cv.Optional("force_dhw"): switch.switch_schema(EcodanSwitch, icon="mdi:water-plus"),
        cv.Optional("holiday_mode"): switch.switch_schema(EcodanSwitch, icon="mdi:beach"),

        # SERVER CONTROL MODE SWITCHES
        # Server Control Mode must be ON for any prohibit command to take effect.
        # The prohibit switches below automatically keep SCM=ON in the same packet
        # when they are toggled, so you do not need to manually enable SCM first.
        # Use the server_control_mode switch to explicitly disable SCM when done.
        #
        # State read-back: all switches read their current status from the 0x28
        # response (at the same packet index used by the write command), so the
        # switch state in HA will reflect what the FTC has actually applied.
        cv.Optional("server_control_mode"): switch.switch_schema(EcodanSwitch, icon="mdi:server-network"),

        # Prohibit DHW: stops the FTC from heating domestic hot water.
        cv.Optional("prohibit_dhw"): switch.switch_schema(EcodanSwitch, icon="mdi:water-off"),

        # Prohibit Heating Zone 1: stops space heating on zone 1.
        cv.Optional("prohibit_heating_zone1"): switch.switch_schema(EcodanSwitch, icon="mdi:radiator-disabled"),

        # Prohibit Cooling Zone 1: stops space cooling on zone 1.
        cv.Optional("prohibit_cooling_zone1"): switch.switch_schema(EcodanSwitch, icon="mdi:snowflake-off"),

        # Prohibit Heating Zone 2: stops space heating on zone 2.
        cv.Optional("prohibit_heating_zone2"): switch.switch_schema(EcodanSwitch, icon="mdi:radiator-disabled"),

        # Prohibit Cooling Zone 2: stops space cooling on zone 2.
        cv.Optional("prohibit_cooling_zone2"): switch.switch_schema(EcodanSwitch, icon="mdi:snowflake-off"),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    heatpump = await cg.get_variable(config[CONF_ECODAN_ID])

    switches = []
    for key, conf in config.items():
        if not isinstance(conf, dict):
            continue
        if CONF_ID not in conf:
            continue
        if conf[CONF_ID].type != EcodanSwitch:
            continue
        var = await switch.new_switch(conf)
        cg.add(getattr(heatpump, f"set_{key}")(var))
        cg.add(var.set_key(key))
        switches.append(f"F({key})")

    cg.add_define(
        "ECODAN_SWITCH_LIST(F, sep)", cg.RawExpression(" sep ".join(switches))
    )