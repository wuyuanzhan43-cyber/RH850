/* pwm9.h - RH850/F1KM-S1 9-channel PWM driver (CS+ / CCRH850)
 *
 * Target pins:
 *   P10_2, P10_0, P0_2, P10_3, P0_6, P10_14, P8_5, P10_6, P10_8
 *
 * Frequency range: 200 kHz - 400 kHz
 * Duty range:      0 - 1000 permille (0.0% - 100.0%)
 *
 * The clock macros below are maximum values from the F1KM-S1 electrical
 * characteristics. Replace them with the actual clock-tree values of your
 * project if the clocks are divided down.
 */
#ifndef PWM9_H
#define PWM9_H

#include <stdint.h>

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
#define PWM_PCLK_IPERI2_HZ  40000000UL   /* TAUB0, PWBA/PWGA/PWSA */
#endif

#ifndef PWM_PCLK_ATAUJ_HZ
#define PWM_PCLK_ATAUJ_HZ   40000000UL   /* TAUJ0, TAUJ2 */
#endif

/* Configure all nine pins and start every channel at 250 kHz / 50%. */
void pwm9_init(void);

/* Set frequency and duty of one channel.
 * freq_hz: 200000 .. 400000
 * duty_permille: 0 .. 1000
 * Returns 0 on success, -1 on error.
 */
int pwm9_set_freq_duty(pwm_ch_t ch, uint32_t freq_hz, uint16_t duty_permille);

/* Update duty only. The frequency is kept unchanged.
 * For TAUD/TAUB/TAUJ the new value is applied at the next period.
 * For PWGA35 the new value is applied at the next falling edge.
 */
int pwm9_set_duty(pwm_ch_t ch, uint16_t duty_permille);

/* Read back the actual generated frequency/duty (after integer rounding). */
uint32_t pwm9_get_actual_freq(pwm_ch_t ch);
uint16_t pwm9_get_actual_duty(pwm_ch_t ch);

#endif /* PWM9_H */
