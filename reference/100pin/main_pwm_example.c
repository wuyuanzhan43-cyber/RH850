/* main_pwm_example.c - CS+ / CCRH850 example
 *
 * Call pwm9_init() after the clock tree, module clocks and ports are ready.
 * In a Smart Configurator project this is normally after R_SystemInit().
 */
#include "pwm9.h"

void main(void)
{
    /* TODO: enable the module clocks for TAUD0, TAUB0, TAUJ0..3,
     *       PWBA0, PWGA35 and PWSA0 (Smart Configurator or MSTPCR).
     */

    pwm9_init();   /* 250 kHz, 50% on all nine channels */

    /* P10_6: 350 kHz, 25% */
    (void)pwm9_set_freq_duty(PWM_P10_6, 350000UL, 250U);

    /* P10_14: keep frequency, change duty to 75% */
    (void)pwm9_set_duty(PWM_P10_14, 750U);

    /* P10_0: 400 kHz, 10% */
    (void)pwm9_set_freq_duty(PWM_P10_0, 400000UL, 100U);

    for (;;) {
        /* application loop */
    }
}
