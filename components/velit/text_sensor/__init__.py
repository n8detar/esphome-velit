import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from .. import VelitClient, VelitHub, register_velit_child, velit_ns
from ..const import CONF_FAULT_TEXT, CONF_VELIT_ID

DEPENDENCIES = ["velit"]

VelitTextSensor = velit_ns.class_(
    "VelitTextSensor", text_sensor.TextSensor, cg.Component, VelitClient
)
VelitTextSensorKind = velit_ns.enum("VelitTextSensorKind")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
        cv.Required(CONF_FAULT_TEXT): text_sensor.text_sensor_schema(
            VelitTextSensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config[CONF_FAULT_TEXT])
    await cg.register_component(var, {})
    await register_velit_child(var, config[CONF_VELIT_ID])
    cg.add(var.set_text_sensor_kind(VelitTextSensorKind.TEXT_SENSOR_KIND_FAULT_TEXT))
