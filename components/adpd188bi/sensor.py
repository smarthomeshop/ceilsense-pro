import esphome.codegen as cg
from esphome.components import binary_sensor, i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_SMOKE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
)

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["binary_sensor"]

CONF_BASELINE_ALPHA = "baseline_alpha"
CONF_BASELINE_READY = "baseline_ready"
CONF_BASELINE_SAMPLES = "baseline_samples"
CONF_ALARM_CANDIDATE = "alarm_candidate"
CONF_ALARM_CONFIRMATIONS = "alarm_confirmations"
CONF_BLUE = "blue"
CONF_BLUE_BASELINE = "blue_baseline"
CONF_BLUE_DELTA = "blue_delta"
CONF_DEVICE_ID = "device_id"
CONF_EFUSE_MODULE_ID = "efuse_module_id"
CONF_BLUE_CALIBRATION_GAIN = "blue_calibration_gain"
CONF_INFRARED_CALIBRATION_GAIN = "infrared_calibration_gain"
CONF_CALIBRATION_READY = "calibration_ready"
CONF_APPLY_EFUSE_CALIBRATION = "apply_efuse_calibration"
CONF_INFRARED = "infrared"
CONF_INFRARED_BASELINE = "infrared_baseline"
CONF_INFRARED_DELTA = "infrared_delta"
CONF_RATIO = "ratio"
CONF_MEASUREMENT_STABLE = "measurement_stable"
CONF_MIN_CHANNEL_RISE = "min_channel_rise"
CONF_MAX_SAMPLE_SPREAD = "max_sample_spread"
CONF_MAX_BASELINE_DRIFT = "max_baseline_drift"
CONF_SAMPLES_PER_WINDOW = "samples_per_window"
CONF_CLEAR_CONFIRMATIONS = "clear_confirmations"
CONF_SMOKE_DETECTED = "smoke_detected"
CONF_SMOKE_INDEX = "smoke_index"
CONF_SMOKE_INDEX_THRESHOLD = "smoke_index_threshold"

adpd188bi_ns = cg.esphome_ns.namespace("adpd188bi")
ADPD188BIComponent = adpd188bi_ns.class_(
    "ADPD188BIComponent", cg.PollingComponent, i2c.I2CDevice
)

COUNT_SENSOR_SCHEMA = sensor.sensor_schema(
    unit_of_measurement="counts",
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
)

DIAGNOSTIC_COUNT_SENSOR_SCHEMA = sensor.sensor_schema(
    unit_of_measurement="counts",
    accuracy_decimals=0,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    state_class=STATE_CLASS_MEASUREMENT,
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ADPD188BIComponent),
            cv.Optional(CONF_BLUE): COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_INFRARED): COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_BLUE_BASELINE): DIAGNOSTIC_COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_INFRARED_BASELINE): DIAGNOSTIC_COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_BLUE_DELTA): DIAGNOSTIC_COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_INFRARED_DELTA): DIAGNOSTIC_COUNT_SENSOR_SCHEMA,
            cv.Optional(CONF_RATIO): sensor.sensor_schema(
                accuracy_decimals=3,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SMOKE_INDEX): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DEVICE_ID): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_EFUSE_MODULE_ID): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_BLUE_CALIBRATION_GAIN): sensor.sensor_schema(
                accuracy_decimals=3,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_INFRARED_CALIBRATION_GAIN): sensor.sensor_schema(
                accuracy_decimals=3,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_SMOKE_DETECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_SMOKE
            ),
            cv.Optional(CONF_MEASUREMENT_STABLE): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_ALARM_CANDIDATE): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_BASELINE_READY): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_CALIBRATION_READY): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_APPLY_EFUSE_CALIBRATION, default=True): cv.boolean,
            cv.Optional(CONF_SMOKE_INDEX_THRESHOLD, default=50.0): cv.float_range(
                min=0.0, max=10000.0
            ),
            cv.Optional(CONF_BASELINE_SAMPLES, default=36): cv.int_range(
                min=1, max=512
            ),
            cv.Optional(CONF_BASELINE_ALPHA, default=0.02): cv.float_range(
                min=0.0, max=1.0
            ),
            cv.Optional(CONF_SAMPLES_PER_WINDOW, default=15): cv.int_range(
                min=5, max=15
            ),
            cv.Optional(CONF_MAX_SAMPLE_SPREAD, default=30.0): cv.float_range(
                min=1.0, max=500.0
            ),
            cv.Optional(CONF_MIN_CHANNEL_RISE, default=15.0): cv.float_range(
                min=0.0, max=10000.0
            ),
            cv.Optional(CONF_MAX_BASELINE_DRIFT, default=20.0): cv.float_range(
                min=1.0, max=500.0
            ),
            cv.Optional(CONF_ALARM_CONFIRMATIONS, default=4): cv.int_range(
                min=1, max=255
            ),
            cv.Optional(CONF_CLEAR_CONFIRMATIONS, default=3): cv.int_range(
                min=1, max=255
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(i2c.i2c_device_schema(0x64))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_smoke_index_threshold(config[CONF_SMOKE_INDEX_THRESHOLD]))
    cg.add(var.set_baseline_samples(config[CONF_BASELINE_SAMPLES]))
    cg.add(var.set_baseline_alpha(config[CONF_BASELINE_ALPHA]))
    cg.add(var.set_samples_per_window(config[CONF_SAMPLES_PER_WINDOW]))
    cg.add(var.set_max_sample_spread(config[CONF_MAX_SAMPLE_SPREAD]))
    cg.add(var.set_min_channel_rise(config[CONF_MIN_CHANNEL_RISE]))
    cg.add(var.set_max_baseline_drift(config[CONF_MAX_BASELINE_DRIFT]))
    cg.add(var.set_alarm_confirmations(config[CONF_ALARM_CONFIRMATIONS]))
    cg.add(var.set_clear_confirmations(config[CONF_CLEAR_CONFIRMATIONS]))
    cg.add(var.set_apply_efuse_calibration(config[CONF_APPLY_EFUSE_CALIBRATION]))

    if CONF_BLUE in config:
        sens = await sensor.new_sensor(config[CONF_BLUE])
        cg.add(var.set_blue_sensor(sens))

    if CONF_INFRARED in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED])
        cg.add(var.set_infrared_sensor(sens))

    if CONF_BLUE_BASELINE in config:
        sens = await sensor.new_sensor(config[CONF_BLUE_BASELINE])
        cg.add(var.set_blue_baseline_sensor(sens))

    if CONF_INFRARED_BASELINE in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED_BASELINE])
        cg.add(var.set_infrared_baseline_sensor(sens))

    if CONF_BLUE_DELTA in config:
        sens = await sensor.new_sensor(config[CONF_BLUE_DELTA])
        cg.add(var.set_blue_delta_sensor(sens))

    if CONF_INFRARED_DELTA in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED_DELTA])
        cg.add(var.set_infrared_delta_sensor(sens))

    if CONF_RATIO in config:
        sens = await sensor.new_sensor(config[CONF_RATIO])
        cg.add(var.set_ratio_sensor(sens))

    if CONF_SMOKE_INDEX in config:
        sens = await sensor.new_sensor(config[CONF_SMOKE_INDEX])
        cg.add(var.set_smoke_index_sensor(sens))

    if CONF_DEVICE_ID in config:
        sens = await sensor.new_sensor(config[CONF_DEVICE_ID])
        cg.add(var.set_device_id_sensor(sens))

    if CONF_EFUSE_MODULE_ID in config:
        sens = await sensor.new_sensor(config[CONF_EFUSE_MODULE_ID])
        cg.add(var.set_efuse_module_id_sensor(sens))

    if CONF_BLUE_CALIBRATION_GAIN in config:
        sens = await sensor.new_sensor(config[CONF_BLUE_CALIBRATION_GAIN])
        cg.add(var.set_blue_calibration_gain_sensor(sens))

    if CONF_INFRARED_CALIBRATION_GAIN in config:
        sens = await sensor.new_sensor(config[CONF_INFRARED_CALIBRATION_GAIN])
        cg.add(var.set_infrared_calibration_gain_sensor(sens))

    if CONF_SMOKE_DETECTED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_SMOKE_DETECTED])
        cg.add(var.set_smoke_detected_binary_sensor(sens))

    if CONF_MEASUREMENT_STABLE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_MEASUREMENT_STABLE])
        cg.add(var.set_measurement_stable_binary_sensor(sens))

    if CONF_ALARM_CANDIDATE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ALARM_CANDIDATE])
        cg.add(var.set_alarm_candidate_binary_sensor(sens))

    if CONF_BASELINE_READY in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BASELINE_READY])
        cg.add(var.set_baseline_ready_binary_sensor(sens))

    if CONF_CALIBRATION_READY in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CALIBRATION_READY])
        cg.add(var.set_calibration_ready_binary_sensor(sens))
