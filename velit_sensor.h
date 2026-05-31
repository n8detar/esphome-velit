import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv

from .. import VelitClient, VelitHub, register_velit_child, velit_ns
from ..const import CONF_OPERATING_MODE, CONF_VELIT_ID, OPERATING_MODE_OPTIONS

DEPENDENCIES = ["velit"]

VelitSelect = velit_ns.class_("VelitSelect", select.Select, cg.Component, VelitClient)
VelitSelectKind = velit_ns.enum("VelitSelectKind")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
        cv.Required(CONF_OPERATING_MODE): select.select_schema(VelitSelect),
    }
)


async def to_code(config):
    var = await select.new_select(
        config[CONF_OPERATING_MODE], options=OPERATING_MODE_OPTIONS
    )
    await cg.register_component(var, {})
    await register_velit_child(var, config[CONF_VELIT_ID])
    cg.add(var.set_select_kind(VelitSelectKind.SELECT_KIND_OPERATING_MODE))
