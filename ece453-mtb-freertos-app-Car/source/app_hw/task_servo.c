#include "task_servo.h"

#include "cybsp.h"
#include "main.h"
#include "task_console.h"
#include <stdbool.h>
#include <string.h>

static cyhal_pwm_t servo_pwm;
static bool servo_initialized = false;

static BaseType_t cli_handler_servo(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

static const CLI_Command_Definition_t xServo =
{
    "servo",
    "\r\nservo <left|right|neutral>:\r\n Drive the steering servo to the requested position\r\n",
    cli_handler_servo,
    1
};

/*
 * Initializes the TCPWM-driven PWM on SERVO_PWM_PIN at 50 Hz and
 * parks the servo at the neutral 1.5 ms pulse (7.5% duty).
 */
cy_rslt_t task_servo_init(void)
{
    cy_rslt_t rslt;

    rslt = cyhal_pwm_init(&servo_pwm, SERVO_PWM_PIN, NULL);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }

    rslt = cyhal_pwm_set_duty_cycle(&servo_pwm,
                                    SERVO_DUTY_NEUTRAL,
                                    SERVO_PWM_FREQ_HZ);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }

    rslt = cyhal_pwm_start(&servo_pwm);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }

    servo_initialized = true;

    FreeRTOS_CLIRegisterCommand(&xServo);

    return CY_RSLT_SUCCESS;
}

/*
 * Sets the servo pulse width based on requested position (~55 deg each side,
 * outside the rated 90 deg window but inside the 500-2500 us max-travel range).
 * LEFT/RIGHT duty values are swapped from the raw datasheet geometry so
 * the requested direction matches physical motion on this build.
 *   SERVO_POS_LEFT    -> 2.1111 ms (10.556%)
 *   SERVO_POS_NEUTRAL -> 1.5    ms (7.5%)
 *   SERVO_POS_RIGHT   -> 0.8889 ms (4.444%)
 */
cy_rslt_t task_servo_set_pulse(servo_position_t pos)
{
    float duty;

    if (!servo_initialized)
    {
        return CYHAL_PWM_RSLT_BAD_ARGUMENT;
    }

    switch (pos)
    {
        case SERVO_POS_LEFT:
            duty = SERVO_DUTY_LEFT;
            break;
        case SERVO_POS_RIGHT:
            duty = SERVO_DUTY_RIGHT;
            break;
        case SERVO_POS_NEUTRAL:
        default:
            duty = SERVO_DUTY_NEUTRAL;
            break;
    }

    return cyhal_pwm_set_duty_cycle(&servo_pwm, duty, SERVO_PWM_FREQ_HZ);
}

/*
 * CLI handler for "servo <left|right|neutral>".
 * Mirrors the joystick-driven path in the GATT handler by calling
 * task_servo_set_pulse() with the matching position.
 */
static BaseType_t cli_handler_servo(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *param;
    BaseType_t param_len;

    configASSERT(pcWriteBuffer);

    param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);

    if (param != NULL && strncmp(param, "left", param_len) == 0)
    {
        task_servo_set_pulse(SERVO_POS_LEFT);
        snprintf(pcWriteBuffer, xWriteBufferLen, "Servo LEFT\r\n");
    }
    else if (param != NULL && strncmp(param, "right", param_len) == 0)
    {
        task_servo_set_pulse(SERVO_POS_RIGHT);
        snprintf(pcWriteBuffer, xWriteBufferLen, "Servo RIGHT\r\n");
    }
    else if (param != NULL && strncmp(param, "neutral", param_len) == 0)
    {
        task_servo_set_pulse(SERVO_POS_NEUTRAL);
        snprintf(pcWriteBuffer, xWriteBufferLen, "Servo NEUTRAL\r\n");
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Usage: servo <left|right|neutral>\r\n");
    }

    return pdFALSE;
}
