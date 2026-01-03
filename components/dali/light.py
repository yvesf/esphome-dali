import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.components.light import LightType
from . import Bus, dali_ns

Output = dali_ns.class_("Output", light.LightOutput)


DEPENDENCIES = ["dali"]
AUTO_LOAD = ["light"]
CONF_BUS= "bus"
CONF_SHORT_ADDRESS = "short_address"
CONFIG_SCHEMA = light.light_schema(
    Output, type_=LightType.BRIGHTNESS_ONLY
).extend(
    {
        cv.Required(CONF_BUS): cv.use_id(Bus),
        cv.Required(CONF_SHORT_ADDRESS): cv.int_,
    }
)

async def to_code(config):
    var = await light.new_light(config)
    bus = await cg.get_variable(config[CONF_BUS])
    cg.add(var.set_bus(bus))

    shortAddress = config[CONF_SHORT_ADDRESS]
    cg.add(var.set_short_address(shortAddress))
