#ifndef TASK_DC_MOTOR_H_
#define TASK_DC_MOTOR_H_

#include "cyhal.h"
#include <stdbool.h>

/* =========================
 * Motor pin assignments
 * ========================= */
#define MOTOR_PWM_PIN   P9_5   /* IO_1 -> blue wire  */
#define MOTOR_DIR_PIN   P9_3   /* IO_0 -> yellow wire */

/* PWM frequency from datasheet: typical 15kHz-25kHz */
#define MOTOR_PWM_FREQ_HZ   20000.0f

/* =========================
 * Public API
 * ========================= */
cy_rslt_t task_dc_motor_init(void);
void task_dc_motor_set_direction(bool ccw);
cy_rslt_t task_dc_motor_set_speed(float speed_percent);
cy_rslt_t task_dc_motor_stop(void);

#endif /* TASK_DC_MOTOR_H_ */