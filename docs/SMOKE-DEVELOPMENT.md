# ADPD188BI optical aerosol development

> This implementation is experimental and is **not a certified smoke alarm**.
> It must never replace a certified EN 14604 smoke alarm or be relied upon for
> life safety.

## Measurement path

The ADPD188BI pulses a blue 470 nm LED and an infrared 850 nm LED. Particles in
the optical chamber scatter light onto the photodiodes. The local ESPHome
component reads both channels, applies the sensor's eFuse calibration values
when available and groups repeated samples into filtered measurement windows.

For every valid window the component:

1. rejects zero, out-of-range or excessively scattered samples;
2. builds an initial clean-air baseline;
3. compares blue and infrared measurements with their baselines;
4. requires both channels to rise before creating an alarm candidate;
5. confirms several consecutive candidates before publishing an alert;
6. updates the clean-air baseline slowly only when no alarm is active.

The published smoke index is a relative development metric. It is not a
standardized obscuration measurement and cannot be compared directly with
commercial detector sensitivity specifications.

## Current development settings

- 15 optical samples per window
- 5 second published update interval
- 36 stable windows before the baseline is armed
- maximum 30% sample spread inside a window
- minimum 15% rise on both optical channels
- 50% smoke-index candidate threshold
- four candidate confirmations to start an alert
- three clear confirmations to release an alert

ESPHome delayed filters add another 5 seconds before exposing an alert and 15
seconds before clearing it at the API level. These conservative values reduce
bench false alarms but are not certified alarm response parameters.

## Chamber dependency

Optical response depends strongly on chamber geometry, surface colour, vents,
airflow and the position of the sensor. Analog Devices' reference work uses a
defined chamber geometry. A custom 3D-printed chamber changes the response and
requires new characterization and thresholds.

Weak response to test aerosol can therefore be caused by poor aerosol entry or
optical geometry even when the sensor electronics are operating normally.

## Development testing

- Use purpose-made detector test aerosol according to its safety instructions.
- Test in a ventilated area and avoid saturating or wetting the sensor.
- Record clean-air baseline, blue/IR values, deltas, ratio, smoke index and
  exact test timing.
- Repeat tests at several distances, orientations and controlled aerosol doses.
- Include nuisance sources such as steam and dust during algorithm research.
- Keep a certified smoke alarm installed during all experiments.

Do not tune a production threshold from one board or one spray test. A serious
validation programme needs multiple boards, environmental conditions, chamber
samples and standardized fire tests through an accredited laboratory.

## Reference material

- Analog Devices ADPD188BI product page and data sheet
- Analog Devices AN-2033 smoke sensor calibration application note
- Analog Devices CN0583 smoke detector reference design
- ESPHome component source in `components/adpd188bi/`
