#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include <array>

namespace esphome {
namespace adpd188bi {

class ADPD188BIComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // On-demand functional test (like a smoke alarm test button): verifies
  // live I2C communication and that both optical channels return signal.
  bool perform_self_test();

  void set_blue_sensor(sensor::Sensor *sensor) { blue_sensor_ = sensor; }
  void set_infrared_sensor(sensor::Sensor *sensor) { infrared_sensor_ = sensor; }
  void set_blue_baseline_sensor(sensor::Sensor *sensor) { blue_baseline_sensor_ = sensor; }
  void set_infrared_baseline_sensor(sensor::Sensor *sensor) { infrared_baseline_sensor_ = sensor; }
  void set_blue_delta_sensor(sensor::Sensor *sensor) { blue_delta_sensor_ = sensor; }
  void set_infrared_delta_sensor(sensor::Sensor *sensor) { infrared_delta_sensor_ = sensor; }
  void set_ratio_sensor(sensor::Sensor *sensor) { ratio_sensor_ = sensor; }
  void set_smoke_index_sensor(sensor::Sensor *sensor) { smoke_index_sensor_ = sensor; }
  void set_device_id_sensor(sensor::Sensor *sensor) { device_id_sensor_ = sensor; }
  void set_smoke_detected_binary_sensor(binary_sensor::BinarySensor *sensor) { smoke_detected_binary_sensor_ = sensor; }
  void set_measurement_stable_binary_sensor(binary_sensor::BinarySensor *sensor) {
    measurement_stable_binary_sensor_ = sensor;
  }
  void set_alarm_candidate_binary_sensor(binary_sensor::BinarySensor *sensor) {
    alarm_candidate_binary_sensor_ = sensor;
  }
  void set_baseline_ready_binary_sensor(binary_sensor::BinarySensor *sensor) {
    baseline_ready_binary_sensor_ = sensor;
  }

  void set_smoke_index_threshold(float threshold) { smoke_index_threshold_ = threshold; }
  void set_baseline_samples(uint16_t samples) { baseline_samples_ = samples; }
  void set_baseline_alpha(float alpha) { baseline_alpha_ = alpha; }
  void set_samples_per_window(uint8_t samples) { samples_per_window_ = samples; }
  void set_max_sample_spread(float spread) { this->max_sample_spread_ = spread; }
  void set_min_channel_rise(float rise) { this->min_channel_rise_ = rise; }
  void set_max_baseline_drift(float drift) { this->max_baseline_drift_ = drift; }
  void set_alarm_confirmations(uint8_t confirmations) { alarm_confirmations_ = confirmations; }
  void set_clear_confirmations(uint8_t confirmations) { clear_confirmations_ = confirmations; }
  void set_apply_efuse_calibration(bool enabled) { apply_efuse_calibration_ = enabled; }
  void set_efuse_module_id_sensor(sensor::Sensor *sensor) { efuse_module_id_sensor_ = sensor; }
  void set_blue_calibration_gain_sensor(sensor::Sensor *sensor) { blue_calibration_gain_sensor_ = sensor; }
  void set_infrared_calibration_gain_sensor(sensor::Sensor *sensor) { infrared_calibration_gain_sensor_ = sensor; }
  void set_calibration_ready_binary_sensor(binary_sensor::BinarySensor *sensor) {
    calibration_ready_binary_sensor_ = sensor;
  }

 protected:
  bool initialize_();
  bool configure_smoke_detection_();
  bool read_efuse_calibration_();
  bool read_u16_(uint8_t reg, uint16_t *value);
  bool write_u16_(uint8_t reg, uint16_t value);
  bool read_sample_(uint32_t *blue, uint32_t *infrared);
  uint32_t calibrate_blue_(uint32_t raw) const;
  uint32_t calibrate_infrared_(uint32_t raw) const;
  bool read_filtered_sample_(uint32_t *blue, uint32_t *infrared, float *blue_spread, float *infrared_spread);
  void take_sample_();
  void finish_sample_window_();
  void publish_alarm_state_(bool state);
  void publish_measurement_stable_(bool state);
  void publish_baseline_ready_(bool state);
  void publish_calibration_state_(bool state);
  void publish_nan_();

  sensor::Sensor *blue_sensor_{nullptr};
  sensor::Sensor *infrared_sensor_{nullptr};
  sensor::Sensor *blue_baseline_sensor_{nullptr};
  sensor::Sensor *infrared_baseline_sensor_{nullptr};
  sensor::Sensor *blue_delta_sensor_{nullptr};
  sensor::Sensor *infrared_delta_sensor_{nullptr};
  sensor::Sensor *ratio_sensor_{nullptr};
  sensor::Sensor *smoke_index_sensor_{nullptr};
  sensor::Sensor *device_id_sensor_{nullptr};
  sensor::Sensor *efuse_module_id_sensor_{nullptr};
  sensor::Sensor *blue_calibration_gain_sensor_{nullptr};
  sensor::Sensor *infrared_calibration_gain_sensor_{nullptr};
  binary_sensor::BinarySensor *smoke_detected_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *measurement_stable_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *alarm_candidate_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *baseline_ready_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *calibration_ready_binary_sensor_{nullptr};

  float smoke_index_threshold_{50.0f};
  float baseline_alpha_{0.02f};
  float max_sample_spread_{30.0f};
  float min_channel_rise_{15.0f};
  float max_baseline_drift_{20.0f};
  uint16_t baseline_samples_{36};
  uint16_t baseline_count_{0};
  uint8_t samples_per_window_{15};
  uint8_t alarm_confirmations_{4};
  uint8_t clear_confirmations_{3};
  uint8_t alarm_candidate_count_{0};
  uint8_t clear_count_{0};
  float blue_baseline_{NAN};
  float infrared_baseline_{NAN};
  bool baseline_ready_{false};
  bool alarm_active_{false};
  bool initialized_{false};
  bool apply_efuse_calibration_{true};
  bool calibration_ready_{false};
  uint8_t read_fail_count_{0};
  uint8_t efuse_module_id_{0};
  uint16_t device_id_{0};
  float blue_calibration_gain_{1.0f};
  float infrared_calibration_gain_{1.0f};

  static constexpr uint8_t MAX_SAMPLES_PER_WINDOW = 15;
  std::array<uint32_t, MAX_SAMPLES_PER_WINDOW> window_blue_samples_{};
  std::array<uint32_t, MAX_SAMPLES_PER_WINDOW> window_infrared_samples_{};
  uint8_t window_sample_count_{0};
  bool window_ready_{false};
  uint32_t window_blue_{0};
  uint32_t window_infrared_{0};
  float window_blue_spread_{NAN};
  float window_infrared_spread_{NAN};

};

}  // namespace adpd188bi
}  // namespace esphome
