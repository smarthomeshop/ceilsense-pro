#include "adpd188bi.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace esphome {
namespace adpd188bi {

static const char *const TAG = "adpd188bi";

static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_DEVID = 0x08;
static const uint8_t REG_SW_RESET = 0x0F;
static const uint8_t REG_MODE = 0x10;
static const uint8_t REG_SLOT_EN = 0x11;
static const uint8_t REG_SAMPLE_CLK = 0x4B;
static const uint8_t REG_DATA_ACCESS_CTL = 0x5F;
static const uint8_t REG_EFUSE_ACCESS = 0x57;
static const uint8_t REG_EFUSE_STATUS = 0x67;
static const uint8_t REG_CLK32M_ADJUST = 0x4D;
static const uint8_t REG_A_PD1_LOW = 0x70;
static const uint8_t REG_A_PD1_HIGH = 0x74;
static const uint8_t REG_B_PD1_LOW = 0x78;
static const uint8_t REG_B_PD1_HIGH = 0x7C;
static const uint8_t EFUSE_DATA_BYTES = 14;
static const uint8_t EFUSE_MODULE_ID_30 = 0x1E;
static const uint8_t EFUSE_MODULE_ID_31 = 0x1F;
static const uint8_t EFUSE_MODULE_ID_33 = 0x21;

// Early ADPD188BI samples report 0x0916. Current production silicon reports
// 0x0A16; the low byte (0x16) identifies the ADPD188BI, the high byte is the
// silicon revision.
static const uint16_t DEVICE_ID_ADPD188BI_REV_A = 0x0916;
static const uint16_t DEVICE_ID_ADPD188BI_REV_B = 0x0A16;
static const uint16_t MODE_PROGRAM = 0x0001;
static const uint16_t MODE_NORMAL = 0x0002;
static const uint16_t SAMPLE_CLK_32K_EN = 0x0080;
static const uint16_t STATUS_CLEAR_FIFO = 0x8000;
static const uint16_t DATA_ACCESS_DIGITAL_CLOCK_EN = 0x0001;
static const uint16_t DATA_ACCESS_SLOT_DATA_HOLD = 0x0006;
// ADI datasheet Table 26: enable both slots and write a 32-bit sum to the
// data registers used by the smoke detector configuration.
static const uint16_t SLOT_EN_DIRECT_32BIT = 0x30A9;
static const uint8_t FILTER_MAX_SAMPLES_PER_WINDOW = 15;
static const uint16_t SAMPLE_INTERVAL_MS = 60;
static const uint32_t MAX_27_BIT_SAMPLE = 0x07FFFFFFUL;

struct RegisterValue {
  uint8_t reg;
  uint16_t value;
};

static bool is_adpd188bi_device_id(uint16_t device_id) {
  return device_id == DEVICE_ID_ADPD188BI_REV_A || device_id == DEVICE_ID_ADPD188BI_REV_B;
}

// ADI AN-2033 uses a truncated 120,112 Hamming code plus global parity for
// the 14-byte eFuse block. Keep the map local so the calibration path can
// reject corrupted coefficients before they affect measured values.
static const uint8_t EFUSE_PARITY_MAP[112] = {
    131, 133, 134, 135, 137, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 149, 150, 151, 152, 153,
    154, 155, 156, 157, 158, 159, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
    175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 193, 194, 195,
    196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
    216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
    236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
};

static uint8_t generate_efuse_block_parity(const std::array<uint8_t, EFUSE_DATA_BYTES> &data) {
  uint8_t parity = 0;
  for (uint8_t byte = 0; byte < EFUSE_DATA_BYTES; byte++) {
    for (uint8_t bit = 0; bit < 8; bit++) {
      if ((data[byte] & (1U << bit)) != 0)
        parity ^= EFUSE_PARITY_MAP[(byte << 3) + bit];
    }
  }
  return parity;
}

static uint8_t fix_efuse_parity(std::array<uint8_t, EFUSE_DATA_BYTES> *data, uint8_t *parity) {
  uint8_t calculated = generate_efuse_block_parity(*data);
  for (uint8_t bit = 0; bit < 6; bit++) {
    if ((*parity & (1U << bit)) != 0)
      calculated ^= 0x80;
  }

  const uint8_t syndrome = (*parity ^ calculated) & 0x7F;
  const bool global_parity_error = ((*parity ^ calculated) & 0x80) != 0;
  if (!global_parity_error)
    return syndrome == 0 ? 0 : 3;
  if (syndrome >= 120)
    return 3;

  switch (syndrome) {
    case 0:
      *parity ^= 0x80;
      return 2;
    case 1:
      *parity ^= 0x01;
      return 2;
    case 2:
      *parity ^= 0x02;
      return 2;
    case 4:
      *parity ^= 0x04;
      return 2;
    case 8:
      *parity ^= 0x08;
      return 2;
    case 16:
      *parity ^= 0x10;
      return 2;
    case 32:
      *parity ^= 0x20;
      return 2;
    case 64:
      *parity ^= 0x40;
      return 2;
    default:
      break;
  }

  const uint8_t adjusted_syndrome = syndrome > 64   ? syndrome - 8
                                    : syndrome > 32 ? syndrome - 7
                                    : syndrome > 16 ? syndrome - 6
                                    : syndrome > 8  ? syndrome - 5
                                    : syndrome > 4  ? syndrome - 4
                                                   : 0;
  const uint8_t byte = adjusted_syndrome >> 3;
  const uint8_t bit = adjusted_syndrome & 0x07;
  if (byte >= EFUSE_DATA_BYTES)
    return 3;
  (*data)[byte] ^= 1U << bit;
  return 1;
}

static const RegisterValue SMOKE_DETECT_CONFIG[] = {
    // Table 26 in the datasheet configures a 32-bit FIFO. ESPHome polls this
    // component every few seconds, not at the 16 Hz FIFO rate. Enabling FIFO
    // overrun prevention would stop new samples after the small FIFO fills.
    // Keep both time slots active and read their direct 32-bit registers.
    {REG_SLOT_EN, SLOT_EN_DIRECT_32BIT}, {0x12, 0x0200}, {0x14, 0x011D}, {0x15, 0x0000}, {0x17, 0x0009},
    {0x18, 0x0000}, {0x19, 0x3FFF}, {0x1A, 0x3FFF}, {0x1B, 0x3FFF}, {0x1D, 0x0009},
    {0x1E, 0x0000}, {0x1F, 0x3FFF}, {0x20, 0x3FFF}, {0x21, 0x3FFF}, {0x22, 0x3539},
    {0x23, 0x3536}, {0x24, 0x1530}, {0x25, 0x630C}, {0x30, 0x0320}, {0x31, 0x040E},
    {0x35, 0x0320}, {0x36, 0x040E}, {0x39, 0x22F0}, {0x3B, 0x22F0}, {0x3C, 0x31C6},
    {0x42, 0x1C34}, {0x43, 0xADA5}, {0x44, 0x1C34}, {0x45, 0xADA5}, {0x54, 0x0AA0},
    {0x58, 0x0544},
};

bool ADPD188BIComponent::read_u16_(uint8_t reg, uint16_t *value) { return this->read_byte_16(reg, value); }

bool ADPD188BIComponent::write_u16_(uint8_t reg, uint16_t value) { return this->write_byte_16(reg, value); }

void ADPD188BIComponent::setup() {
  // Never mark_failed(): that permanently stops updates. A smoke sensor
  // must keep retrying so it recovers from boot-time glitches or a rail
  // that comes up late, and keeps logging the exact failure.
  if (!this->initialize_()) {
    ESP_LOGW(TAG, "ADPD188BI not responding at setup, retrying every update interval");
    this->status_set_error();
  }
}

bool ADPD188BIComponent::initialize_() {
  uint16_t devid = 0;
  if (!this->read_u16_(REG_DEVID, &devid)) {
    ESP_LOGE(TAG, "Failed to read device id (no I2C ACK from 0x%02X)", this->address_);
    return false;
  }
  this->device_id_ = devid;
  if (this->device_id_sensor_ != nullptr)
    this->device_id_sensor_->publish_state(devid);

  if (!is_adpd188bi_device_id(devid)) {
    ESP_LOGE(TAG, "Unexpected device id 0x%04X, expected ADPD188BI 0x%04X or 0x%04X", devid,
             DEVICE_ID_ADPD188BI_REV_A, DEVICE_ID_ADPD188BI_REV_B);
    return false;
  }

  if (!this->write_u16_(REG_SW_RESET, 0x0001)) {
    ESP_LOGE(TAG, "Software reset failed");
    return false;
  }
  delay(10);

  if (!this->read_efuse_calibration_()) {
    ESP_LOGW(TAG, "ADPD188BI eFuse calibration not applied; using uncalibrated relative counts");
  }

  if (!this->configure_smoke_detection_()) {
    ESP_LOGE(TAG, "Smoke detection register setup failed");
    return false;
  }

  // Restart the baseline warm-up so a re-init after an outage settles cleanly.
  this->baseline_ready_ = false;
  this->baseline_count_ = 0;
  this->read_fail_count_ = 0;
  this->alarm_candidate_count_ = 0;
  this->clear_count_ = 0;
  this->alarm_active_ = false;
  this->window_sample_count_ = 0;
  this->window_ready_ = false;
  this->publish_alarm_state_(false);
  this->publish_measurement_stable_(false);
  this->publish_baseline_ready_(false);
  this->initialized_ = true;
  this->status_clear_error();
  this->set_interval("adpd188bi_sample", SAMPLE_INTERVAL_MS, [this]() { this->take_sample_(); });
  ESP_LOGI(TAG, "ADPD188BI smoke sensor ready (device id 0x%04X)", devid);
  return true;
}

bool ADPD188BIComponent::read_efuse_calibration_() {
  this->calibration_ready_ = false;
  this->efuse_module_id_ = 0;
  this->blue_calibration_gain_ = 1.0f;
  this->infrared_calibration_gain_ = 1.0f;
  this->publish_calibration_state_(false);

  if (!this->apply_efuse_calibration_)
    return false;

  bool ok = true;
  bool efuse_access_enabled = false;
  bool fifo_clock_enabled = false;
  bool efuse_refresh_complete = false;
  uint16_t sample_clk = 0;
  std::array<uint8_t, EFUSE_DATA_BYTES> efuse_data{};
  uint8_t efuse_parity = 0;

  if (!this->read_u16_(REG_SAMPLE_CLK, &sample_clk)) {
    ESP_LOGW(TAG, "ADPD188BI eFuse: failed to read 32 kHz clock register");
    ok = false;
  }
  if (ok && !this->write_u16_(REG_SAMPLE_CLK, sample_clk | SAMPLE_CLK_32K_EN)) {
    ESP_LOGW(TAG, "ADPD188BI eFuse: failed to enable 32 kHz oscillator");
    ok = false;
  }
  if (ok && !this->write_u16_(REG_MODE, MODE_PROGRAM)) {
    ESP_LOGW(TAG, "ADPD188BI eFuse: failed to enter program mode");
    ok = false;
  }
  if (ok) {
    if (this->write_u16_(REG_DATA_ACCESS_CTL, DATA_ACCESS_DIGITAL_CLOCK_EN)) {
      fifo_clock_enabled = true;
    } else {
      ESP_LOGW(TAG, "ADPD188BI eFuse: failed to enable 32 MHz FIFO clock");
      ok = false;
    }
  }
  if (ok) {
    if (this->write_u16_(REG_EFUSE_ACCESS, 0x0007)) {
      efuse_access_enabled = true;
    } else {
      ESP_LOGW(TAG, "ADPD188BI eFuse: failed to enable eFuse access");
      ok = false;
    }
  }

  if (ok) {
    for (uint8_t attempt = 0; attempt < 100; attempt++) {
      uint16_t status = 0;
      if (!this->read_u16_(REG_EFUSE_STATUS, &status)) {
        ESP_LOGW(TAG, "ADPD188BI eFuse: failed to read refresh status");
        ok = false;
        break;
      }
      if ((status & 0x00FF) == 0x04) {
        efuse_refresh_complete = true;
        break;
      }
      delay(1);
    }
    if (ok && !efuse_refresh_complete) {
      ESP_LOGW(TAG, "ADPD188BI eFuse: refresh timeout");
      ok = false;
    }
  }

  if (ok) {
    for (uint8_t reg = 0x70; reg <= 0x7E; reg++) {
      uint16_t value = 0;
      if (!this->read_u16_(reg, &value)) {
        ESP_LOGW(TAG, "ADPD188BI eFuse: failed to read register 0x%02X", reg);
        ok = false;
        break;
      }
      if (reg == 0x7E) {
        efuse_parity = static_cast<uint8_t>(value & 0x00FF);
      } else {
        efuse_data[reg - 0x70] = static_cast<uint8_t>(value & 0x00FF);
      }
    }
  }

  if (ok) {
    const uint8_t ecc_result = fix_efuse_parity(&efuse_data, &efuse_parity);
    if (ecc_result == 3) {
      ESP_LOGW(TAG, "ADPD188BI eFuse: ECC reported multiple bit errors");
      ok = false;
    } else if (ecc_result != 0) {
      ESP_LOGW(TAG, "ADPD188BI eFuse: corrected a single-bit ECC error");
    }
  }

  if (ok) {
    this->efuse_module_id_ = efuse_data[0];
    const bool module_30_or_31 = this->efuse_module_id_ == EFUSE_MODULE_ID_30 ||
                                  this->efuse_module_id_ == EFUSE_MODULE_ID_31;
    const bool module_33_or_newer = this->efuse_module_id_ >= EFUSE_MODULE_ID_33;
    if (!module_30_or_31 && !module_33_or_newer) {
      ESP_LOGW(TAG, "ADPD188BI eFuse: unsupported module id 0x%02X", this->efuse_module_id_);
      ok = false;
    } else {
      // These are the actual LED currents from the register values written in
      // SMOKE_DETECT_CONFIG, not an assumed 200 mA nominal setting.
      const uint16_t led3_register = 0x3539;
      const uint16_t led1_register = 0x3536;
      const uint16_t led_fine_register = 0x630C;
      const float led3_current =
          (50.3f + 19.8f * static_cast<float>(led3_register & 0x000F)) *
          (0.74f + 0.022f * static_cast<float>((led_fine_register >> 11) & 0x001F)) *
          (0.1f + 0.9f * static_cast<float>((led3_register >> 13) & 0x0001));
      const float led1_current =
          (50.3f + 19.8f * static_cast<float>(led1_register & 0x000F)) *
          (0.74f + 0.022f * static_cast<float>(led_fine_register & 0x001F)) *
          (0.1f + 0.9f * static_cast<float>((led1_register >> 13) & 0x0001));

      const float blue_mean_gain = module_30_or_31 ? 17.0f : 21.0f;
      const float infrared_mean_gain = module_30_or_31 ? 34.0f : 42.0f;
      const float blue_intercept_offset = module_30_or_31 ? 128.0f : 80.0f;
      const float infrared_intercept_offset = module_30_or_31 ? 128.0f : 80.0f;
      const float blue_mean_intercept = module_30_or_31 ? 622.0f : 753.0f;
      const float infrared_mean_intercept = module_30_or_31 ? 128.0f : 156.0f;

      const float blue_gain = (blue_mean_gain / 256.0f) * (static_cast<float>(efuse_data[1]) - 112.0f) +
                              blue_mean_gain;
      const float infrared_gain =
          (infrared_mean_gain / 256.0f) * (static_cast<float>(efuse_data[2]) - 112.0f) + infrared_mean_gain;
      const float blue_intercept =
          8.0f * (static_cast<float>(efuse_data[3]) - blue_intercept_offset);
      const float infrared_intercept =
          5.0f * (static_cast<float>(efuse_data[4]) - infrared_intercept_offset);
      const float blue_nominal_scalar = blue_mean_gain * led1_current + blue_mean_intercept;
      const float infrared_nominal_scalar = infrared_mean_gain * led3_current + infrared_mean_intercept;
      const float blue_device_scalar = blue_gain * led1_current + blue_intercept;
      const float infrared_device_scalar = infrared_gain * led3_current + infrared_intercept;
      const float blue_calibration = blue_device_scalar / blue_nominal_scalar;
      const float infrared_calibration = infrared_device_scalar / infrared_nominal_scalar;

      if (!std::isfinite(blue_calibration) || !std::isfinite(infrared_calibration) || blue_calibration < 0.25f ||
          blue_calibration > 4.0f || infrared_calibration < 0.25f || infrared_calibration > 4.0f) {
        ESP_LOGW(TAG, "ADPD188BI eFuse: calculated gain outside safe range");
        ok = false;
      } else {
        this->blue_calibration_gain_ = blue_calibration;
        this->infrared_calibration_gain_ = infrared_calibration;

        if (module_33_or_newer) {
          // Module 33 stores factory-optimal clock trim values in the same
          // eFuse block. Keep the oscillator enable bit set after copying.
          if (!this->write_u16_(REG_SAMPLE_CLK, static_cast<uint16_t>(efuse_data[7]) | SAMPLE_CLK_32K_EN) ||
              !this->write_u16_(REG_CLK32M_ADJUST, static_cast<uint16_t>(efuse_data[8]))) {
            ESP_LOGW(TAG, "ADPD188BI eFuse: failed to apply oscillator trim");
            ok = false;
          }
        }
      }
    }
  }

  if (efuse_access_enabled && !this->write_u16_(REG_EFUSE_ACCESS, 0x0000))
    ESP_LOGW(TAG, "ADPD188BI eFuse: failed to disable eFuse access");
  if (fifo_clock_enabled && !this->write_u16_(REG_DATA_ACCESS_CTL, 0x0000))
    ESP_LOGW(TAG, "ADPD188BI eFuse: failed to disable 32 MHz FIFO clock");

  if (ok) {
    this->calibration_ready_ = true;
    ESP_LOGI(TAG, "ADPD188BI eFuse calibration active (module 0x%02X, blue gain %.3f, IR gain %.3f)",
             this->efuse_module_id_, this->blue_calibration_gain_, this->infrared_calibration_gain_);
  } else {
    this->efuse_module_id_ = 0;
    this->blue_calibration_gain_ = 1.0f;
    this->infrared_calibration_gain_ = 1.0f;
  }
  this->publish_calibration_state_(this->calibration_ready_);
  return this->calibration_ready_;
}

bool ADPD188BIComponent::configure_smoke_detection_() {
  uint16_t sample_clk = 0;
  if (!this->read_u16_(REG_SAMPLE_CLK, &sample_clk))
    return false;
  if (!this->write_u16_(REG_SAMPLE_CLK, sample_clk | SAMPLE_CLK_32K_EN))
    return false;

  uint16_t data_access = 0;
  if (!this->read_u16_(REG_DATA_ACCESS_CTL, &data_access))
    return false;
  if (!this->write_u16_(REG_DATA_ACCESS_CTL, (data_access | DATA_ACCESS_DIGITAL_CLOCK_EN) & ~DATA_ACCESS_SLOT_DATA_HOLD))
    return false;
  if (!this->write_u16_(REG_MODE, MODE_PROGRAM))
    return false;
  delay(2);

  for (const auto &item : SMOKE_DETECT_CONFIG) {
    if (!this->write_u16_(item.reg, item.value))
      return false;
  }

  if (!this->write_u16_(REG_STATUS, STATUS_CLEAR_FIFO))
    return false;
  if (!this->write_u16_(REG_MODE, MODE_NORMAL))
    return false;

  return true;
}

void ADPD188BIComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ADPD188BI Optical Smoke Sensor");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
  ESP_LOGCONFIG(TAG, "  Device ID: 0x%04X", this->device_id_);
  ESP_LOGCONFIG(TAG, "  Baseline samples: %u", this->baseline_samples_);
  ESP_LOGCONFIG(TAG, "  Baseline alpha: %.3f", this->baseline_alpha_);
  ESP_LOGCONFIG(TAG, "  Samples per window: %u", this->samples_per_window_);
  ESP_LOGCONFIG(TAG, "  Max sample spread: %.1f %%", this->max_sample_spread_);
  ESP_LOGCONFIG(TAG, "  Min channel rise: %.1f %%", this->min_channel_rise_);
  ESP_LOGCONFIG(TAG, "  Max baseline drift: %.1f %%", this->max_baseline_drift_);
  ESP_LOGCONFIG(TAG, "  Alarm confirmations: %u", this->alarm_confirmations_);
  ESP_LOGCONFIG(TAG, "  Smoke index threshold: %.1f %%", this->smoke_index_threshold_);
  ESP_LOGCONFIG(TAG, "  Apply eFuse calibration: %s", this->apply_efuse_calibration_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  eFuse calibration ready: %s", this->calibration_ready_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  eFuse module id: 0x%02X", this->efuse_module_id_);
  ESP_LOGCONFIG(TAG, "  Calibration gains: blue %.3f, IR %.3f", this->blue_calibration_gain_,
                this->infrared_calibration_gain_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Blue", this->blue_sensor_);
  LOG_SENSOR("  ", "Infrared", this->infrared_sensor_);
  LOG_SENSOR("  ", "Blue Baseline", this->blue_baseline_sensor_);
  LOG_SENSOR("  ", "Infrared Baseline", this->infrared_baseline_sensor_);
  LOG_SENSOR("  ", "Blue Delta", this->blue_delta_sensor_);
  LOG_SENSOR("  ", "Infrared Delta", this->infrared_delta_sensor_);
  LOG_SENSOR("  ", "Blue/IR Ratio", this->ratio_sensor_);
  LOG_SENSOR("  ", "Smoke Index", this->smoke_index_sensor_);
  LOG_SENSOR("  ", "Device ID", this->device_id_sensor_);
  LOG_SENSOR("  ", "eFuse Module ID", this->efuse_module_id_sensor_);
  LOG_SENSOR("  ", "Blue Calibration Gain", this->blue_calibration_gain_sensor_);
  LOG_SENSOR("  ", "Infrared Calibration Gain", this->infrared_calibration_gain_sensor_);
  LOG_BINARY_SENSOR("  ", "Smoke Detected", this->smoke_detected_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Measurement Stable", this->measurement_stable_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Alarm Candidate", this->alarm_candidate_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Baseline Ready", this->baseline_ready_binary_sensor_);
}

bool ADPD188BIComponent::read_sample_(uint32_t *blue, uint32_t *infrared) {
  uint16_t blue_low = 0;
  uint16_t blue_high = 0;
  uint16_t ir_low = 0;
  uint16_t ir_high = 0;
  uint16_t data_access = 0;

  if (!this->read_u16_(REG_DATA_ACCESS_CTL, &data_access))
    return false;
  if (!this->write_u16_(REG_DATA_ACCESS_CTL, data_access | DATA_ACCESS_DIGITAL_CLOCK_EN | DATA_ACCESS_SLOT_DATA_HOLD))
    return false;

  const bool ok = this->read_u16_(REG_A_PD1_LOW, &blue_low) && this->read_u16_(REG_A_PD1_HIGH, &blue_high) &&
                  this->read_u16_(REG_B_PD1_LOW, &ir_low) && this->read_u16_(REG_B_PD1_HIGH, &ir_high);

  this->write_u16_(REG_DATA_ACCESS_CTL, (data_access | DATA_ACCESS_DIGITAL_CLOCK_EN) & ~DATA_ACCESS_SLOT_DATA_HOLD);

  if (!ok)
    return false;

  // The extended registers contain a 27-bit value. Masking the unused high
  // bits prevents a stale/reserved bit from turning into a false large signal.
  *blue = ((uint32_t(blue_high) & 0x07FFUL) << 16) | blue_low;
  *infrared = ((uint32_t(ir_high) & 0x07FFUL) << 16) | ir_low;
  return true;
}

uint32_t ADPD188BIComponent::calibrate_blue_(uint32_t raw) const {
  if (!this->calibration_ready_)
    return raw;
  const float calibrated = static_cast<float>(raw) / std::max(0.25f, this->blue_calibration_gain_);
  if (!std::isfinite(calibrated) || calibrated <= 0.0f)
    return 0;
  return static_cast<uint32_t>(std::min(calibrated, static_cast<float>(MAX_27_BIT_SAMPLE)) + 0.5f);
}

uint32_t ADPD188BIComponent::calibrate_infrared_(uint32_t raw) const {
  if (!this->calibration_ready_)
    return raw;
  const float calibrated = static_cast<float>(raw) / std::max(0.25f, this->infrared_calibration_gain_);
  if (!std::isfinite(calibrated) || calibrated <= 0.0f)
    return 0;
  return static_cast<uint32_t>(std::min(calibrated, static_cast<float>(MAX_27_BIT_SAMPLE)) + 0.5f);
}

void ADPD188BIComponent::take_sample_() {
  if (!this->initialized_ || this->window_ready_)
    return;

  uint32_t blue = 0;
  uint32_t infrared = 0;
  if (!this->read_sample_(&blue, &infrared) || blue == 0 || infrared == 0 || blue > MAX_27_BIT_SAMPLE ||
      infrared > MAX_27_BIT_SAMPLE) {
    if (this->read_fail_count_ < 255)
      this->read_fail_count_++;
    if (this->read_fail_count_ >= 10) {
      ESP_LOGW(TAG, "ADPD188BI sample stream lost; restarting sensor");
      this->cancel_interval("adpd188bi_sample");
      this->initialized_ = false;
      this->window_sample_count_ = 0;
      this->window_ready_ = false;
      this->publish_measurement_stable_(false);
      this->status_set_error();
    }
    return;
  }

  blue = this->calibrate_blue_(blue);
  infrared = this->calibrate_infrared_(infrared);
  if (blue == 0 || infrared == 0)
    return;

  this->read_fail_count_ = 0;
  this->window_blue_samples_[this->window_sample_count_] = blue;
  this->window_infrared_samples_[this->window_sample_count_] = infrared;
  this->window_sample_count_++;
  if (this->window_sample_count_ >= this->samples_per_window_)
    this->finish_sample_window_();
}

void ADPD188BIComponent::finish_sample_window_() {
  const uint8_t count = this->window_sample_count_;
  const uint8_t minimum_valid_samples = (this->samples_per_window_ * 2) / 3 + 1;
  if (count < minimum_valid_samples) {
    ESP_LOGW(TAG, "Unstable measurement window: only %u/%u valid samples", count, this->samples_per_window_);
    this->window_sample_count_ = 0;
    return;
  }

  std::sort(this->window_blue_samples_.begin(), this->window_blue_samples_.begin() + count);
  std::sort(this->window_infrared_samples_.begin(), this->window_infrared_samples_.begin() + count);

  const uint8_t median_index = count / 2;
  const uint8_t lower_quartile_index = count / 4;
  const uint8_t upper_quartile_index = (count * 3) / 4;
  this->window_blue_ = this->window_blue_samples_[median_index];
  this->window_infrared_ = this->window_infrared_samples_[median_index];
  this->window_blue_spread_ =
      (static_cast<float>(this->window_blue_samples_[upper_quartile_index] -
                          this->window_blue_samples_[lower_quartile_index]) /
       std::max(1.0f, static_cast<float>(this->window_blue_))) *
      100.0f;
  this->window_infrared_spread_ =
      (static_cast<float>(this->window_infrared_samples_[upper_quartile_index] -
                          this->window_infrared_samples_[lower_quartile_index]) /
       std::max(1.0f, static_cast<float>(this->window_infrared_))) *
      100.0f;
  this->window_sample_count_ = 0;

  if (this->window_blue_spread_ > this->max_sample_spread_ ||
      this->window_infrared_spread_ > this->max_sample_spread_) {
    ESP_LOGW(TAG, "Unstable measurement window (blue spread %.1f %%, IR spread %.1f %%)", this->window_blue_spread_,
             this->window_infrared_spread_);
    this->window_ready_ = false;
    return;
  }
  this->window_ready_ = true;
}

bool ADPD188BIComponent::read_filtered_sample_(uint32_t *blue, uint32_t *infrared, float *blue_spread,
                                                float *infrared_spread) {
  std::array<uint32_t, FILTER_MAX_SAMPLES_PER_WINDOW> blue_samples{};
  std::array<uint32_t, FILTER_MAX_SAMPLES_PER_WINDOW> infrared_samples{};
  uint8_t count = 0;

  for (uint8_t sample = 0; sample < this->samples_per_window_; sample++) {
    uint32_t raw_blue = 0;
    uint32_t raw_infrared = 0;
    if (this->read_sample_(&raw_blue, &raw_infrared) && raw_blue > 0 && raw_infrared > 0 &&
        raw_blue <= MAX_27_BIT_SAMPLE && raw_infrared <= MAX_27_BIT_SAMPLE) {
      const uint32_t calibrated_blue = this->calibrate_blue_(raw_blue);
      const uint32_t calibrated_infrared = this->calibrate_infrared_(raw_infrared);
      if (calibrated_blue == 0 || calibrated_infrared == 0)
        continue;
      blue_samples[count] = calibrated_blue;
      infrared_samples[count] = calibrated_infrared;
      count++;
    }
    if (sample + 1 < this->samples_per_window_)
      delay(SAMPLE_INTERVAL_MS);
  }

  const uint8_t minimum_valid_samples = (this->samples_per_window_ * 2) / 3 + 1;
  if (count < minimum_valid_samples) {
    ESP_LOGW(TAG, "Unstable measurement window: only %u/%u valid samples", count, this->samples_per_window_);
    return false;
  }

  std::sort(blue_samples.begin(), blue_samples.begin() + count);
  std::sort(infrared_samples.begin(), infrared_samples.begin() + count);

  const uint8_t median_index = count / 2;
  const uint8_t lower_quartile_index = count / 4;
  const uint8_t upper_quartile_index = (count * 3) / 4;
  *blue = blue_samples[median_index];
  *infrared = infrared_samples[median_index];
  *blue_spread = (static_cast<float>(blue_samples[upper_quartile_index] - blue_samples[lower_quartile_index]) /
                  std::max(1.0f, static_cast<float>(*blue))) *
                 100.0f;
  *infrared_spread =
      (static_cast<float>(infrared_samples[upper_quartile_index] - infrared_samples[lower_quartile_index]) /
       std::max(1.0f, static_cast<float>(*infrared))) *
      100.0f;

  if (*blue_spread > this->max_sample_spread_ || *infrared_spread > this->max_sample_spread_) {
    ESP_LOGW(TAG, "Unstable measurement window (blue spread %.1f %%, IR spread %.1f %%)", *blue_spread,
             *infrared_spread);
    return false;
  }
  return true;
}

void ADPD188BIComponent::publish_alarm_state_(bool state) {
  this->alarm_active_ = state;
  if (this->smoke_detected_binary_sensor_ != nullptr)
    this->smoke_detected_binary_sensor_->publish_state(state);
}

void ADPD188BIComponent::publish_measurement_stable_(bool state) {
  if (this->measurement_stable_binary_sensor_ != nullptr)
    this->measurement_stable_binary_sensor_->publish_state(state);
}

void ADPD188BIComponent::publish_baseline_ready_(bool state) {
  if (this->baseline_ready_binary_sensor_ != nullptr)
    this->baseline_ready_binary_sensor_->publish_state(state);
}

void ADPD188BIComponent::publish_calibration_state_(bool state) {
  if (this->efuse_module_id_sensor_ != nullptr)
    this->efuse_module_id_sensor_->publish_state(this->efuse_module_id_);
  if (this->blue_calibration_gain_sensor_ != nullptr)
    this->blue_calibration_gain_sensor_->publish_state(this->blue_calibration_gain_);
  if (this->infrared_calibration_gain_sensor_ != nullptr)
    this->infrared_calibration_gain_sensor_->publish_state(this->infrared_calibration_gain_);
  if (this->calibration_ready_binary_sensor_ != nullptr)
    this->calibration_ready_binary_sensor_->publish_state(state);
}

void ADPD188BIComponent::update() {
  if (!this->initialized_) {
    if (!this->initialize_()) {
      this->status_set_error();
      this->publish_nan_();
      return;
    }
  }

  if (!this->window_ready_) {
    this->publish_measurement_stable_(false);
    if (this->alarm_candidate_binary_sensor_ != nullptr)
      this->alarm_candidate_binary_sensor_->publish_state(false);
    this->alarm_candidate_count_ = 0;
    this->clear_count_ = 0;
    return;
  }

  const uint32_t blue = this->window_blue_;
  const uint32_t infrared = this->window_infrared_;
  this->window_ready_ = false;
  this->window_sample_count_ = 0;
  this->status_clear_warning();
  this->publish_measurement_stable_(true);

  const float blue_value = static_cast<float>(blue);
  const float infrared_value = static_cast<float>(infrared);

  if (this->blue_sensor_ != nullptr)
    this->blue_sensor_->publish_state(blue_value);
  if (this->infrared_sensor_ != nullptr)
    this->infrared_sensor_->publish_state(infrared_value);

  if (!this->baseline_ready_) {
    if (this->baseline_count_ == 1) {
      // The initial sample is set below together with a restarted baseline.
    } else {
      const float blue_drift = std::abs(blue_value - this->blue_baseline_) / std::max(1.0f, this->blue_baseline_) * 100.0f;
      const float infrared_drift =
          std::abs(infrared_value - this->infrared_baseline_) / std::max(1.0f, this->infrared_baseline_) * 100.0f;
      if (blue_drift > this->max_baseline_drift_ || infrared_drift > this->max_baseline_drift_) {
        ESP_LOGW(TAG, "Baseline changed during warm-up (blue %.1f %%, IR %.1f %%); restarting warm-up", blue_drift,
                 infrared_drift);
        this->baseline_count_ = 0;
      }
    }

    this->baseline_count_++;
    if (this->baseline_count_ == 1) {
      this->blue_baseline_ = blue_value;
      this->infrared_baseline_ = infrared_value;
    } else {
      const float count = static_cast<float>(this->baseline_count_);
      this->blue_baseline_ += (blue_value - this->blue_baseline_) / count;
      this->infrared_baseline_ += (infrared_value - this->infrared_baseline_) / count;
    }

    if (this->baseline_count_ >= this->baseline_samples_) {
      this->baseline_ready_ = true;
      ESP_LOGI(TAG, "Baseline stabilized after %u windows; smoke detection armed", this->baseline_count_);
    }

    if (this->blue_baseline_sensor_ != nullptr)
      this->blue_baseline_sensor_->publish_state(this->blue_baseline_);
    if (this->infrared_baseline_sensor_ != nullptr)
      this->infrared_baseline_sensor_->publish_state(this->infrared_baseline_);
    if (this->smoke_index_sensor_ != nullptr)
      this->smoke_index_sensor_->publish_state(0.0f);
    this->publish_baseline_ready_(this->baseline_ready_);
    this->publish_alarm_state_(false);
    if (this->alarm_candidate_binary_sensor_ != nullptr)
      this->alarm_candidate_binary_sensor_->publish_state(false);
    return;
  }

  const float blue_delta = blue_value - this->blue_baseline_;
  const float infrared_delta = infrared_value - this->infrared_baseline_;
  const float positive_blue_delta = std::max(0.0f, blue_delta);
  const float positive_infrared_delta = std::max(0.0f, infrared_delta);
  const float blue_norm = positive_blue_delta / std::max(1.0f, this->blue_baseline_);
  const float infrared_norm = positive_infrared_delta / std::max(1.0f, this->infrared_baseline_);
  const float smoke_index = std::max(blue_norm, infrared_norm) * 100.0f;
  const float ratio = infrared > 0 ? blue_value / infrared_value : NAN;
  const float blue_percent = blue_norm * 100.0f;
  const float infrared_percent = infrared_norm * 100.0f;
  const bool alarm_candidate = smoke_index >= this->smoke_index_threshold_ &&
                               blue_percent >= this->min_channel_rise_ &&
                               infrared_percent >= this->min_channel_rise_;

  if (smoke_index < this->min_channel_rise_) {
    this->blue_baseline_ += this->baseline_alpha_ * (blue_value - this->blue_baseline_);
    this->infrared_baseline_ += this->baseline_alpha_ * (infrared_value - this->infrared_baseline_);
  }

  if (this->alarm_candidate_binary_sensor_ != nullptr)
    this->alarm_candidate_binary_sensor_->publish_state(alarm_candidate);
  if (alarm_candidate) {
    this->alarm_candidate_count_ = std::min<uint8_t>(this->alarm_candidate_count_ + 1, this->alarm_confirmations_);
    this->clear_count_ = 0;
    if (this->alarm_candidate_count_ >= this->alarm_confirmations_)
      this->publish_alarm_state_(true);
  } else {
    this->alarm_candidate_count_ = 0;
    if (this->alarm_active_) {
      this->clear_count_ = std::min<uint8_t>(this->clear_count_ + 1, this->clear_confirmations_);
      if (this->clear_count_ >= this->clear_confirmations_)
        this->publish_alarm_state_(false);
    } else {
      this->clear_count_ = 0;
    }
  }

  if (this->blue_baseline_sensor_ != nullptr)
    this->blue_baseline_sensor_->publish_state(this->blue_baseline_);
  if (this->infrared_baseline_sensor_ != nullptr)
    this->infrared_baseline_sensor_->publish_state(this->infrared_baseline_);
  if (this->blue_delta_sensor_ != nullptr)
    this->blue_delta_sensor_->publish_state(positive_blue_delta);
  if (this->infrared_delta_sensor_ != nullptr)
    this->infrared_delta_sensor_->publish_state(positive_infrared_delta);
  if (this->ratio_sensor_ != nullptr)
    this->ratio_sensor_->publish_state(ratio);
  if (this->smoke_index_sensor_ != nullptr)
    this->smoke_index_sensor_->publish_state(smoke_index);
}

bool ADPD188BIComponent::perform_self_test() {
  // If the sensor never came up (or dropped out), actively try to bring it
  // back before judging it — the test button doubles as a recovery probe.
  if (!this->initialized_ && !this->initialize_()) {
    ESP_LOGE(TAG, "Self-test failed: sensor not initialized and re-init failed");
    this->status_set_error();
    return false;
  }
  uint16_t devid = 0;
  if (!this->read_u16_(REG_DEVID, &devid) || !is_adpd188bi_device_id(devid)) {
    ESP_LOGE(TAG, "Self-test failed: device id read returned 0x%04X", devid);
    return false;
  }
  uint32_t blue = 0;
  uint32_t infrared = 0;
  float blue_spread = NAN;
  float infrared_spread = NAN;
  if (!this->read_filtered_sample_(&blue, &infrared, &blue_spread, &infrared_spread)) {
    ESP_LOGE(TAG, "Self-test failed: no stable measurement window");
    return false;
  }
  // A working optical front-end always returns some counts (dark offset +
  // LED response); a dead channel reads back zero.
  if (blue == 0 || infrared == 0) {
    ESP_LOGE(TAG, "Self-test failed: optical channel dead (blue=%u, ir=%u)", (unsigned) blue, (unsigned) infrared);
    return false;
  }
  ESP_LOGI(TAG, "Self-test passed (blue=%u, ir=%u)", (unsigned) blue, (unsigned) infrared);
  return true;
}

void ADPD188BIComponent::publish_nan_() {
  if (this->blue_sensor_ != nullptr)
    this->blue_sensor_->publish_state(NAN);
  if (this->infrared_sensor_ != nullptr)
    this->infrared_sensor_->publish_state(NAN);
  if (this->blue_delta_sensor_ != nullptr)
    this->blue_delta_sensor_->publish_state(NAN);
  if (this->infrared_delta_sensor_ != nullptr)
    this->infrared_delta_sensor_->publish_state(NAN);
  if (this->ratio_sensor_ != nullptr)
    this->ratio_sensor_->publish_state(NAN);
  if (this->smoke_index_sensor_ != nullptr)
    this->smoke_index_sensor_->publish_state(NAN);
  if (this->alarm_candidate_binary_sensor_ != nullptr)
    this->alarm_candidate_binary_sensor_->publish_state(false);
}

}  // namespace adpd188bi
}  // namespace esphome
