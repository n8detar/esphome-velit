import esphome.codegen as cg
from esphome.components import ble_client
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .const import CONF_DEVICE_TYPE, CONF_VELIT_ID, DEVICE_TYPE_AC, DEVICE_TYPE_HEATER

CODEOWNERS = []
DEPENDENCIES = ["ble_client"]
MULTI_CONF = True

velit_ns = cg.esphome_ns.namespace("velit")

VelitClient = velit_ns.class_("VelitClient")
VelitHub = velit_ns.class_(
    "VelitHub", ble_client.BLEClientNode, cg.PollingComponent
)

VelitDeviceType = velit_ns.enum("VelitDeviceType")
DEVICE_TYPES = {
    DEVICE_TYPE_AC: VelitDeviceType.DEVICE_TYPE_AC,
    DEVICE_TYPE_HEATER: VelitDeviceType.DEVICE_TYPE_HEATER,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VelitHub),
            cv.Required(CONF_DEVICE_TYPE): cv.enum(DEVICE_TYPES, lower=True),
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

VELIT_CHILD_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VELIT_ID): cv.use_id(VelitHub),
    }
)


async def register_velit_child(var, value):
    parent = await cg.get_variable(value)
    await cg.register_parented(var, value)
    cg.add(parent.register_child(var))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    cg.add(var.set_device_type(config[CONF_DEVICE_TYPE]))
    if config[CONF_DEVICE_TYPE] == VelitDeviceType.DEVICE_TYPE_AC:
        cg.add(var.set_update_interval(6000))
    else:
        cg.add(var.set_update_interval(1000))
