#ifndef TASK_SERVO_H_
#define TASK_SERVO_H_

#include "main.h"
#include "task_console.h"
#include "cyhal.h"
#include <stdbool.h>

/* =========================
 * Servo pin / signal config
 * ========================= */
#define SERVO_PWM_PIN        P7_2     /* TCPWM output to servo signal wire */
#define SERVO_PWM_FREQ_HZ    50.0f    /* 20 ms period */

/* Pulse-width duty cycles at 50 Hz (period = 20 ms).
 * Datasheet: 1000-2000 us spans 90 deg (~11.11 us/deg) is the rated
 * operating travel. Max travel ~180 deg at 500-2500 us is also allowed.
 * Targeting ~55 deg each side from neutral (~611 us offset) — outside
 * the rated 90 deg window but inside the 500-2500 us max-travel range.
 * Left/right duties are swapped from the raw geometry so that
 * SERVO_POS_LEFT physically steers left on this build.
 */
#define SERVO_DUTY_LEFT      10.556f  /* 2.1111 ms -> physical left  */
#define SERVO_DUTY_NEUTRAL   7.5f     /* 1.5    ms -> neutral        */
#define SERVO_DUTY_RIGHT     4.444f   /* 0.8889 ms -> physical right */

typedef enum
{
    SERVO_POS_NEUTRAL = 0,
    SERVO_POS_LEFT,
    SERVO_POS_RIGHT
} servo_position_t;

/* =========================
 * Public API
 * ========================= */
cy_rslt_t task_servo_init(void);
cy_rslt_t task_servo_set_pulse(servo_position_t pos);

#endif /* TASK_SERVO_H_ */
