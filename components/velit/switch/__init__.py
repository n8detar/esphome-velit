import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_SWITCH

from .. import VelitClient, VelitHub, register_velit_child, velit_ns
from ..const import CONF_SWING, CONF_VELIT_ID

DEPENDENCIES = ["velit"]

VelitSwitch = velit_ns.class_("VelitSwitch", switch.Switch, cg.Component, VelitClient)
VelitSwitchKind = velit_ns.enum("VelitSwitchKind")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
        cv.Required(CONF_SWING): switch.switch_schema(
            VelitSwitch,
            device_class=DEVICE_CLASS_SWITCH,
        ),
    }
)


async def to_code(config):
    var = await switch.new_switch(config[CONF_SWING])
    await cg.register_component(var, {})
    await register_velit_child(var, config[CONF_VELIT_ID])
    cg.add(var.set_switch_kind(VelitSwitchKind.SWITCH_KIND_SWING))
