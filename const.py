import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv

from .. import VelitClient, VELIT_CHILD_SCHEMA, register_velit_child, velit_ns
from ..const import CONF_VELIT_ID

DEPENDENCIES = ["velit"]

VelitClimate = velit_ns.class_(
    "VelitClimate", climate.Climate, cg.Component, VelitClient
)

CONFIG_SCHEMA = (
    climate.climate_schema(VelitClimate)
    .extend(VELIT_CHILD_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await register_velit_child(var, config[CONF_VELIT_ID])
