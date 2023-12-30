import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
)

CODEOWNERS = ["@tobias-93"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "switch"]

ecodan_ns = cg.esphome_ns.namespace("ecodan_")
ECODAN = ecodan_ns.class_("EcodanHeatpump", cg.Component, uart.UARTDevice)

CONF_ECODAN_ID = "ecodan_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ECODAN),
    }
).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    uart_component = await cg.get_variable(config[CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_component)
    await cg.register_component(var, config)
