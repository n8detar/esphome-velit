import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import ICON_FAN

from .. import VelitClient, VelitHub, register_velit_child, velit_ns
from ..const import CONF_MANUAL_FAN_SPEED, CONF_VELIT_ID

DEPENDENCIES = ["velit"]

VelitNumber = velit_ns.class_("VelitNumber", number.Number, cg.Component, VelitClient)
VelitNumberKind = velit_ns.enum("VelitNumberKind")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
        cv.Required(CONF_MANUAL_FAN_SPEED): number.number_schema(
            VelitNumber,
            icon=ICON_FAN,
        ),
    }
)


async def to_code(config):
    var = await number.new_number(
        config[CONF_MANUAL_FAN_SPEED], min_value=1, max_value=5, step=1
    )
    await cg.register_component(var, {})
    await register_velit_child(var, config[CONF_VELIT_ID])
    cg.add(var.set_number_kind(VelitNumberKind.NUMBER_KIND_MANUAL_FAN_SPEED))
