/**
 * @file    sketch.ino
 * @brief   ECG Gating System - R-Peak and T-Wave Detection with Empiric Thresholds
 *
 * Extends R-peak detection with T-wave (repolarization) peak detection. Implements
 * adaptive R-peak threshold using signal maximum during adaptation phase (30000 samples).
 * T-wave detection uses empiric voltage thresholds (1.8V-2.56V) converted to ADC bits.
 * Detects local peaks within T-wave window using max tracking algorithm.
 *
 * @note    Arduino hardware: ADC input range 0-1023 bits maps to 0-5V physical input.
 *          ECG signal conditioning circuit constrains output to 0.61V-4.47V range.
 *          Digital output on Pin 11 fires when T-wave peak detected.
 *
 * @author  Arturo Vargas Cuevas (A01652564)
 * @date    2023-03-09
 */

/* ============================================================================
 * Hardware Pin Configuration
 * ============================================================================
 */

/* Arduino analog input pin for ECG signal acquisition */
const int adc_pin = A3;

/* Arduino digital output pin for R-peak trigger */
const int output_pin = 11;

/* ============================================================================
 * ADC Parametrization Constants
 * ============================================================================
 */

/* Minimum valid ECG input voltage from conditioning circuit (V) */
const float adc_min_voltage = 0.610;

/* Maximum valid ECG input voltage from conditioning circuit (V) */
const float adc_max_voltage = 4.470;

/* Total voltage span of the input signal (V) */
float adc_voltage_range;

/* ============================================================================
 * Real-Time Signal Processing State
 * ============================================================================
 */

/* Current ADC sample value (0-1023 bits) */
int ecg_sample;

/* Current digital output state (HIGH/LOW) */
int output_state = LOW;

/* Loop iteration counter for adaptation phase tracking */
int loop_count = 0;

/* Flag indicating adaptation phase completion (30000 samples collected) */
boolean is_adaptation_complete = false;

/* Counter for ADC samples from detected peak (for T-wave timing) */
int adc_sample_counter = 0;

/* ============================================================================
 * Peak Detection Algorithm Variables
 * ============================================================================
 */

/* Running maximum value during current detection window */
int max_value = 0;

/* Latest confirmed absolute maximum (R-peak candidate) */
int absolute_max = 0;

/* Previous maximum value for edge detection */
int previous_max = 0;

/* Flag indicating new absolute maximum was found */
int absolute_max_flag = 0;

/* ============================================================================
 * R-Wave (QRS Complex) Detection Variables
 * ============================================================================
 */

/* Detected R-peak (absolute maximum) value during adaptation */
int r_max = 0;

/* R-peak threshold as percentage of absolute maximum */
const float r_peak_percentage = 0.6;

/* Computed R-peak threshold value for firing */
int r_threshold = 0;

/* ============================================================================
 * T-Wave Detection Threshold Variables
 * ============================================================================
 */

/* T-wave lower voltage threshold (V) - empiric value */
const float t_threshold_min_voltage = 1.8;

/* T-wave upper voltage threshold (V) - empiric value */
const float t_threshold_max_voltage = 2.56;

/* T-wave lower threshold converted to ADC bits (0-1023) */
int t_threshold_min_bit;

/* T-wave upper threshold converted to ADC bits (0-1023) */
int t_threshold_max_bit;

/* Maximum T-wave value tracker during detection window */
int max_t_value = 0;


void setup()
{
    /* Configure digital output pin as active-high trigger */
    pinMode(output_pin, OUTPUT);

    /* Compute ADC voltage-to-bit scaling factor.
     * Signal conditioning circuit constrains all ECG samples to [adc_min_voltage,
     * adc_max_voltage] range. This allows direct threshold comparison on ADC samples.
     */
    adc_voltage_range = adc_max_voltage - adc_min_voltage;

    /* Convert T-wave voltage thresholds to ADC bit domain.
     * Scaling formula: bit_value = (voltage_threshold / adc_range) * 1023
     * ADC has 10-bit resolution (0-1023 discrete levels).
     */
    t_threshold_min_bit = round(t_threshold_min_voltage / (adc_voltage_range / 1023.0));
    t_threshold_max_bit = round(t_threshold_max_voltage / (adc_voltage_range / 1023.0));
}


void loop()
{
    /* Acquire fresh ECG sample from analog conditioning circuit */
    ecg_sample = analogRead(adc_pin);

    /* Increment ADC sample counter for T-wave timing */
    adc_sample_counter++;

    /* Loop counter for adaptation phase tracking */
    if (loop_count < 30000) {
        loop_count++;
    } else {
        is_adaptation_complete = true;
    }

    /* During adaptation phase, determine R-peak threshold from signal maximum */
    if (!is_adaptation_complete) {
        r_max = find_absolute_max(ecg_sample);
        r_threshold = round(r_max * r_peak_percentage);
    }

    /* After adaptation phase, detect R and T peaks */
    if (is_adaptation_complete) {
        /* Find R-peak (absolute maximum crossing threshold) */
        if (ecg_sample > r_threshold) {
            digitalWrite(output_pin, HIGH);
            adc_sample_counter = 0;
        } else {
            digitalWrite(output_pin, LOW);
        }

        /* Find T-wave peak (local maximum in empiric voltage range) */
        if (adc_sample_counter > 500 && adc_sample_counter < 1000) {
            if (ecg_sample > t_threshold_min_bit && ecg_sample < t_threshold_max_bit) {
                if (max_t_value < ecg_sample) {
                    max_t_value = ecg_sample;
                } else {
                    digitalWrite(output_pin, HIGH);
                    max_t_value = 0;
                }
            } else {
                digitalWrite(output_pin, LOW);
            }
        }
    }
}

/* ============================================================================
 * Helper Functions
 * ============================================================================
 */
int find_absolute_max(int ecg_sample)
{
    /* Track running maximum: update if new sample exceeds current max */
    if (ecg_sample > max_value) {
        max_value = ecg_sample;
        previous_max = max_value;
    }

    /* Update absolute maximum when running max changes */
    if (max_value != absolute_max) {
        absolute_max = max_value;
        absolute_max_flag = 1;
    }

    return absolute_max;
}

