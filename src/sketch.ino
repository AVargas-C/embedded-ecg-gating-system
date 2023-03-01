/**
 * @file    sketch.ino
 * @brief   ECG Gating System - Threshold Calibration Exploration (Phase 1)
 *
 * Initial exploration phase testing T-wave threshold conversion from analog voltage
 * to ADC bit values. This prototype validates the ADC parametrization required for
 * subsequent signal processing stages.
 *
 * @note    Arduino hardware: ADC input range 0-1023 bits maps to 0-5V physical input.
 *          ECG signal conditioning circuit scales cardiac signal to 0.61V-4.47V range.
 *
 * @author  Arturo Vargas Cuevas (A01652564)
 * @date    2023-03-01
 *
 * @copyright Copyright (c) 2023 Arturo Vargas Cuevas. All rights reserved.
 * @license   Apache-2.0
 */

/* ============================================================================
 * ADC Parametrization Constants
 * ============================================================================
/**< Minimum valid ECG input voltage from conditioning circuit (V) */
const float adc_min_voltage = 0.610;

/**< Maximum valid ECG input voltage from conditioning circuit (V) */
const float adc_max_voltage = 4.470;

/**< Total voltage span of the input signal (V) */
float adc_voltage_range;

/* ============================================================================
 * T-Wave Detection Threshold Variables
 * ============================================================================
/**< T-wave lower voltage threshold (V) - empirically set */
const float t_threshold_min_voltage = 1.0;

/**< T-wave upper voltage threshold (V) - empirically set */
const float t_threshold_max_voltage = 2.0;

/**< T-wave lower threshold converted to ADC bits (0-1023) */
int t_threshold_min_bit;

/**< T-wave upper threshold converted to ADC bits (0-1023) */
int t_threshold_max_bit;

/* ============================================================================
 * setup()
 * ============================================================================
 */
void setup()
{
    Serial.begin(9600);


    adc_voltage_range = adc_max_voltage - adc_min_voltage;

    /* Convert T-wave voltage thresholds to ADC bit domain.*/
    t_threshold_min_bit = round(
        t_threshold_min_voltage / (adc_voltage_range / 1023.0)
    );
    t_threshold_max_bit = round(
        t_threshold_max_voltage / (adc_voltage_range / 1023.0)
    );
}

/* ============================================================================
 * loop()
 * ============================================================================
 */
void loop()
{
    /* Output minimum threshold bit value to serial monitor */
    Serial.println(t_threshold_min_bit);

    /* Output maximum threshold bit value to serial monitor */
    Serial.println(t_threshold_max_bit);

    delay(1000);
}
