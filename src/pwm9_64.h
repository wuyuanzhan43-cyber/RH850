/* pwm9_64.h - R7F701690 (RH850/F1KM-S1 64-pin) 9-channel PWM driver
 *
 * Target pins (64-pin LQFP):
 *   P10_2, P10_0, P0_2, P10_3, P0_6, P10_14, P8_5, P10_6, P10_8
 *
 * P0_6 has NO timer/PWGA output on the 64-pin package.
 * By default P0_6 is driven by software PWM using OSTM0.
 * Set PWM9_USE_P10_4_FOR_P0_6 to 1 only if the P0_6 net is
 * reworked to P10_4 (TAUD0O9).
 *
 * Frequency range: 200 kHz - 400 kHz
 * Duty range:      0 - 1000 permille (0.0% - 100.0%)
 *
 * The clock macros below are maximum values from the F1KM-S1 electrical
 * characteristics. Replace them with the actual clock-tree values of your
 * project if the clocks are divided down.
 */
#ifndef PWM9_64_H
#define PWM9_64_H

#include <stdint.h>

/* P0_6 has no timer/PWGA output on the 64-pin package.
 * Set to 1 only after reworking the P0_6 net to P10_4 (TAUD0O9).
 */
#ifndef PWM9_USE_P10_4_FOR_P0_6
#define PWM9_USE_P10_4_FOR_P0_6 0
#endif

typedef enum {
    PWM_P10_2 = 0,
    PWM_P10_0,
    PWM_P0_2,
    PWM_P10_3,
    PWM_P0_6,
    PWM_P10_14,
    PWM_P8_5,
    PWM_P10_6,
    PWM_P10_8,
    PWM_CH_COUNT
} pwm_ch_t;

#ifndef PWM_PCLK_IPERI1_HZ
#define PWM_PCLK_IPERI1_HZ  80000000UL   /* TAUD0, TAUJ1, TAUJ3 */
#endif

#ifndef PWM_PCLK_IPERI2_HZ
#define PWM_PCLK_IPERI2_HZ  40000000UL   /* PWBA/PWGA/PWSA */
#endif

#ifndef PWM_PCLK_ATAUJ_HZ
#define PWM_PCLK_ATAUJ_HZ   40000000UL   /* TAUJ0, TAUJ2 */
#endif

/* OSTM0 clock = CPUCLK_L. F1KM-S1 max is 60 MHz. */
#ifndef PWM_SW_PCLK_HZ
#define PWM_SW_PCLK_HZ      60000000UL
#endif

/* Configure all nine pins and start every channel at 250 kHz / 50%. */
void pwm9_init(void);

/* Software PWM ISR for P0_6 (OSTM0 free-run compare).
 * Route the INTOSTM0 vector to this function in your CS+ project.
 */
void pwm_sw_isr(void);

/* Set frequency and duty of one channel.
 * freq_hz: 200000 .. 400000
 * duty_permille: 0 .. 1000
 * Returns 0 on success, -1 on error.
 */
int pwm9_set_freq_duty(pwm_ch_t ch, uint32_t freq_hz, uint16_t duty_permille);

/* Update duty only. The frequency is kept unchanged.
 * For TAUD/TAUJ the new value is applied at the next period.
 * For PWGA19 the new value is applied at the next falling edge.
 * For P0_6 software PWM the new counts are applied at the next edge.
 */
int pwm9_set_duty(pwm_ch_t ch, uint16_t duty_permille);

/* Read back the actual generated frequency/duty (after integer rounding). */
uint32_t pwm9_get_actual_freq(pwm_ch_t ch);
uint16_t pwm9_get_actual_duty(pwm_ch_t ch);

#endif /* PWM9_64_H */
