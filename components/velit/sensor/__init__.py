import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_METER,
)

from .. import VelitClient, VelitHub, register_velit_child, velit_ns
from ..const import CONF_ALTITUDE, CONF_AMBIENT_TEMPERATURE, CONF_FAULT_CODE, CONF_VELIT_ID

DEPENDENCIES = ["velit"]

VelitSensor = velit_ns.class_("VelitSensor", sensor.Sensor, cg.Component, VelitClient)
VelitSensorKind = velit_ns.enum("VelitSensorKind")

SENSOR_KINDS = {
    CONF_AMBIENT_TEMPERATURE: VelitSensorKind.SENSOR_KIND_AMBIENT_TEMPERATURE,
    CONF_FAULT_CODE: VelitSensorKind.SENSOR_KIND_FAULT_CODE,
    CONF_ALTITUDE: VelitSensorKind.SENSOR_KIND_ALTITUDE,
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
            cv.Optional(CONF_AMBIENT_TEMPERATURE): sensor.sensor_schema(
                VelitSensor,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FAULT_CODE): sensor.sensor_schema(
                VelitSensor,
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ALTITUDE): sensor.sensor_schema(
                VelitSensor,
                unit_of_measurement=UNIT_METER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ),
    cv.has_at_least_one_key(CONF_AMBIENT_TEMPERATURE, CONF_FAULT_CODE, CONF_ALTITUDE),
)


async def _build_sensor(conf, parent_id, kind):
    var = await sensor.new_sensor(conf)
    await cg.register_component(var, {})
    await register_velit_child(var, parent_id)
    cg.add(var.set_sensor_kind(kind))


async def to_code(config):
    parent_id = config[CONF_VELIT_ID]
    for key, kind in SENSOR_KINDS.items():
        if conf := config.get(key):
            await _build_sensor(conf, parent_id, kind)
