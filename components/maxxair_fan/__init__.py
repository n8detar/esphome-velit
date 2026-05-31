"""ESPHome external component: MaxxAir Fan IR control via protocol encoding."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_transmitter
from esphome.const import CONF_ID

CONF_TRANSMITTER_ID = "transmitter_id"

maxxair_fan_ns = cg.esphome_ns.namespace("maxxair_fan")
MaxxAirFan = maxxair_fan_ns.class_("MaxxAirFan", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MaxxAirFan),
        cv.Required(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    tx = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(tx))
