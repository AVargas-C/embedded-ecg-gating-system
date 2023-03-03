/**
 * @file    sketch.ino
 * @brief   ECG Gating System - R-Peak Detection Simplified (Phase 3)
 *
 * Simplification of R-peak detection algorithm, reducing threshold percentage
 * from 75% to 60% for more sensitive peak detection. Removes complex R-R interval
 * tracking to streamline algorithm. Focuses on robust R-peak detection without
 * additional statistics collection.
 *
 * @note    Arduino hardware: ADC input range 0-1023 bits maps to 0-5V physical input.
 *          ECG signal conditioning circuit scales cardiac signal to 0.61V-4.47V range.
 *          Digital output on Pin 11 fires when R-peak detected (after adaptation).
 *
 * @author  Arturo Vargas Cuevas (A01652564)
 * @date    2023-03-03
 */

/* ============================================================================
 * Hardware Pin Configuration
 * ============================================================================
 */

/* Arduino analog input pin for ECG signal acquisition */
const int adc_pin = A3;

/* Arduino digital output pin for R-peak detection trigger */
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

/* Flag indicating adaptation phase completion (10000 samples collected) */
boolean is_adaptation_complete = false;

/* ============================================================================
 * Peak Detection Algorithm Variables
 * ============================================================================
 */

/* Local maximum peaks circular buffer size */
const int local_max_buffer_size = 100;

/* Circular buffer storing detected local peak values */
int local_max_peaks[local_max_buffer_size];

/* Current write index into local_max_peaks buffer */
int local_max_index = 0;

/* Intermediate calculation variable for peak detection */
float peak_value = 0.0;

/* Running maximum value during current detection window */
int max_value = 0;

/* Latest confirmed absolute maximum (R-peak candidate) */
int absolute_max = 0;

/* Previous maximum value for edge detection */
int previous_max = 0;

/* Flag indicating new absolute maximum was found */
int absolute_max_flag = 0;

/* Average of recent local peaks */
float average_value = 0.0;

/* ============================================================================
 * R-Wave (QRS Complex) Detection Variables
 * ============================================================================
 */

/* Detected R-peak (absolute maximum) value during adaptation */
int r_max = 0;

/* R-peak threshold as percentage of absolute maximum (tunable) */
const float r_peak_percentage = 0.6;

/* Computed R-peak threshold value for firing */
int r_threshold = 0;

/* ============================================================================
* T-Wave Detection Threshold Variables
* ============================================================================
*/

/* T-wave lower voltage threshold (V) - refined empiric value */
const float t_threshold_min_voltage = 1.8;

/* T-wave upper voltage threshold (V) - refined empiric value */
const float t_threshold_max_voltage = 2.56;

/* T-wave lower threshold converted to ADC bits (0-1023) */
int t_threshold_min_bit;

/* T-wave upper threshold converted to ADC bits (0-1023) */
int t_threshold_max_bit;

/* Maximum T-wave value tracker during detection window */
int max_t_value = 0;


void setup()
{
    /* Initialize serial at 9600 baud for debugging and serial output */
    Serial.begin(9600);

    /* Configure digital output pin as active-high trigger for external hardware */
    pinMode(output_pin, OUTPUT);

    /* Compute ADC voltage-to-bit scaling factor.
     * Signal conditioning circuit constrains all ECG samples to [adc_min_voltage,
     * adc_max_voltage] range. This allows direct threshold comparison on ADC samples.
     */
    adc_voltage_range = adc_max_voltage - adc_min_voltage;

    /* Convert T-wave voltage thresholds to ADC bit domain.
     * Scaling formula: bit_value = (voltage_threshold / adc_range) * 1023
     * Datasheet §4.1: ADC has 10-bit resolution (0-1023 discrete levels).
     */
    t_threshold_min_bit = round(
        t_threshold_min_voltage / (adc_voltage_range / 1023.0)
    );
    t_threshold_max_bit = round(
        t_threshold_max_voltage / (adc_voltage_range / 1023.0)
    );
}


void loop()
{
    /* Acquire fresh ECG sample from analog conditioning circuit */
    ecg_sample = analogRead(adc_pin);

    /* Increment loop counter for adaptation phase tracking.
     * Adaptation period = 10,000 samples ≈ 10 seconds at 1kHz.
     */
    if (loop_count < 10000) {
        loop_count++;
    } else {
        is_adaptation_complete = true;
    }

    if (!is_adaptation_complete) {
        /* During adaptation, continuously track absolute maximum value
         * to establish R-peak threshold as r_peak_percentage of max.
         */
        r_max = find_absolute_max(ecg_sample);
        r_threshold = round(r_max * r_peak_percentage);
    }

    if (is_adaptation_complete) {
        /* Threshold comparison: R-peak detected when sample exceeds threshold */
        if (ecg_sample > r_threshold) {
            digitalWrite(output_pin, HIGH);
        } else {
            digitalWrite(output_pin, LOW);
        }
    }

    Serial.println("ECG Sample: " + String(ecg_sample));
    Serial.println("R Threshold: " + String(r_threshold));
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

    /* Update absolute maximum when running max changes.
     * This indicates a new peak detection window and transition to next beat.
     */
    if (max_value != absolute_max) {
        absolute_max = max_value;
        absolute_max_flag = 1;
    }

    return absolute_max;
}


int find_local_max(int ecg_sample, int absolute_max)
{
    int max_flag = 0;

    /* Detect local peaks: update max when new sample exceeds running max */
    if (ecg_sample > max_value) {
        max_value = ecg_sample;
        previous_max = max_value;
        max_flag = 1;
    }

    /* Save detected local peak to circular buffer */
    if (max_flag == 1) {
        local_max_peaks[local_max_index] = previous_max;
        local_max_index++;
        max_flag = 0;
    }

    /* Wrap circular buffer index when full */
    if (local_max_index == local_max_buffer_size) {
        local_max_index = 0;
    }

    /* Compute average of buffered peaks for adaptive thresholding */
    average_value = compute_array_average(
        local_max_peaks,
        local_max_buffer_size
    );

    return average_value;
}


float compute_array_average(int *array, int length)
{
    long sum = 0L;  /* Long accumulator prevents 16-bit overflow on ATmega328P */

    /* Sum all elements in array */
    for (int i = 0; i < length; i++) {
        sum += array[i];
    }

    /* Return average as floating-point for precision */
    return ((float)sum) / length;
}
