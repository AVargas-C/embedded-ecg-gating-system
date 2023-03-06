/**
 * @file    sketch.ino
 * @brief   ECG Gating System - Basic Threshold Detection (Phase 4 - V1)
 *
 * First production version implementing basic ECG threshold detection.
 * Acquires raw ECG signal and compares against fixed voltage threshold (1.46V).
 * Outputs digital HIGH when signal exceeds threshold, LOW otherwise.
 * Simple finite state logic without adaptation or peak detection.
 *
 * @note    Arduino hardware: ADC input range 0-1023 bits maps to 0-5V physical input.
 *          Uses standard 5V reference for ADC conversion (not conditioned circuit).
 *          Digital output on Pin 6 triggers when threshold exceeded.
 *
 * @author  Arturo Vargas Cuevas (A01652564)
 * @date    2023-03-06
 */

/* ============================================================================
 * Hardware Pin Configuration
 * ============================================================================
 */

/* Arduino analog input pin for ECG signal acquisition */
const int adc_pin = A0;

/* Arduino digital output pin for threshold trigger */
const int output_pin = 6;

/* ============================================================================
 * Threshold Detection Variables
 * ============================================================================
 */

/* Fixed voltage threshold for ECG detection (V) */
const float threshold_voltage = 1.46;

/* Threshold voltage converted to ADC bit value (0-1023) */
int threshold_bit;

/* ============================================================================
 * Real-Time Signal Processing State
 * ============================================================================
 */

/* Current ADC sample value (0-1023 bits) */
int ecg_sample;

/* Current digital output state (HIGH/LOW) */
int output_state = LOW;


void setup()
{
    /* Configure digital output pin as active-high trigger */
    pinMode(output_pin, OUTPUT);

    /* Convert threshold voltage to ADC bit value using standard 5V reference.
     * Scaling formula: bit_value = (voltage_threshold / 5.0) * 1023
     * ADC has 10-bit resolution (0-1023 levels for 0-5V range).
     */
    threshold_bit = round(threshold_voltage / (5.0 / 1023.0));
}


void loop()
{
    /* Read fresh ECG sample from analog input */
    ecg_sample = analogRead(adc_pin);

    /* Simple threshold comparison */
    if (ecg_sample >= threshold_bit) {
        output_state = HIGH;
    }

    if (ecg_sample < threshold_bit) {
        output_state = LOW;
    }

    /* Write output state to digital pin */
    digitalWrite(output_pin, output_state);
}

