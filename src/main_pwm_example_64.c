/* main_pwm_example_64.c - R7F701690 (RH850/F1KM-S1 64-pin) example
 *
 * Call pwm9_init() after the clock tree, module clocks and ports are ready.
 *
 * P0_6 uses software PWM on OSTM0 (free-run compare).
 *   - Route the INTOSTM0 vector to pwm_sw_isr() in CS+.
 *   - If the P0_6 net is reworked to P10_4, set
 *     PWM9_USE_P10_4_FOR_P0_6 to 1 and use hardware TAUD0O9 instead.
 *
 * P10_14 uses PWGA19O (not TAUB0O7) on the 64-pin package.
 */
#include "pwm9_64.h"

void main(void)
{
    /* TODO: enable module clocks for TAUD0, TAUJ0..3,
     *       PWBA0, PWGA19, PWSA0 and OSTM0.
     */

    pwm9_init();   /* 250 kHz, 50% on all nine channels */

    /* P10_6: 350 kHz, 25% (TAUD0 CH12/CH13) */
    (void)pwm9_set_freq_duty(PWM_P10_6, 350000UL, 250U);

    /* P10_14: keep frequency, change duty to 75% (PWGA19) */
    (void)pwm9_set_duty(PWM_P10_14, 750U);

    /* P10_0: 400 kHz, 10% (TAUJ1 CH0/CH3) */
    (void)pwm9_set_freq_duty(PWM_P10_0, 400000UL, 100U);

    /* P0_6: software PWM on OSTM0 (300 kHz, 50%) */
    (void)pwm9_set_freq_duty(PWM_P0_6, 300000UL, 500U);

    for (;;) {
        /* application loop */
    }
}
