#include "task_dc_motor.h"

#include "cybsp.h"
#include <stdbool.h>

static cyhal_pwm_t motor_pwm;
static bool motor_initialized = false;

/*
 * Initializes:
 *  - direction GPIO
 *  - PWM output for motor speed control
 *
 * Datasheet behavior:
 *  - PWM pin is active-low
 *  - High/open = motor OFF
 *  - Low = motor ON
 */

cy_rslt_t task_dc_motor_init(void)
{
    cy_rslt_t rslt;

    /* Direction pin: low = CW, high = CCW */
    cyhal_gpio_init(MOTOR_DIR_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);

   cyhal_gpio_write(MOTOR_DIR_PIN, true);

    /* Initialize PWM on speed pin */
   cyhal_pwm_init(&motor_pwm, MOTOR_PWM_PIN, NULL);

    /*
     * Motor OFF initially.
     * Since PWM input is active-low:
     *   100% high duty => always high => motor OFF
     */
    rslt = cyhal_pwm_set_duty_cycle(&motor_pwm, 100.0f, MOTOR_PWM_FREQ_HZ);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }

    rslt = cyhal_pwm_start(&motor_pwm);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }

    motor_initialized = true;
    return CY_RSLT_SUCCESS;
}

/*
 * ccw = true  -> high  -> CCW
 * ccw = false -> low   -> CW
 */
void task_dc_motor_set_direction(bool ccw)
{

    cyhal_gpio_write(MOTOR_DIR_PIN, ccw);
}


cy_rslt_t task_dc_motor_set_speed(float speed_percent)
{
    float high_duty;

    if (!motor_initialized)
    {
        return CYHAL_PWM_RSLT_BAD_ARGUMENT;
    }

    high_duty = speed_percent;

    return cyhal_pwm_set_duty_cycle(&motor_pwm, high_duty, MOTOR_PWM_FREQ_HZ);
}

cy_rslt_t task_dc_motor_stop(void)
{
    /* active-low input -> 100% high means OFF */
    return cyhal_pwm_set_duty_cycle(&motor_pwm, 100.0f, MOTOR_PWM_FREQ_HZ);
}