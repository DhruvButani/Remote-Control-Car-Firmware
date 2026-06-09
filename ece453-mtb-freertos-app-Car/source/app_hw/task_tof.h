/**
 * @file task_tof.h
 * @author Dhruv Butani
 * @brief
 * FreeRTOS task header for the VL53LX Time-of-Flight distance sensor.
 *
 * Other tasks (or the CLI) post a tof_message_type_t value to q_tof to
 * request a one-shot distance reading.  task_tof() handles the request,
 * waits for data-ready, and prints the result to the serial console.
 *
 * @version 0.1
 * @date 2024-07-11
 *
 * @copyright Copyright (c) 2024
 */

#ifndef __TASK_TOF_H__
#define __TASK_TOF_H__

#include "main.h"
#include "ece453_pins.h"

/* VL53LX driver API */
#include "vl53lx_api.h"
#include "vl53lx_platform.h"
#include "vl53lx_platform_init.h"

/*******************************************************************************
 * Pin definitions (Module 1 connector: SCL=P9_0, SDA=P9_1, XSHUT=P9_4)
 ******************************************************************************/
#define TOF_PIN_SCL         MOD_1_PIN_I2C_SCL   /* P9_0 */
#define TOF_PIN_SDA         MOD_1_PIN_I2C_SDA   /* P9_1 */
#define TOF_PIN_XSHUT       MOD_1_PIN_IO_0      /* P9_4 */

/*******************************************************************************
 * Sensor configuration
 ******************************************************************************/
#define TOF_I2C_ADDR            (0x29U)   /* 7-bit I2C address               */
#define TOF_I2C_SPEED_KHZ       (400U)    /* 400 kHz fast-mode               */
#define TOF_TIMING_BUDGET_US    (50000U)  /* 50 ms: balanced speed/accuracy  */
#define TOF_POLL_DELAY_MS       (10U)     /* polling interval while waiting  */

/*******************************************************************************
 * Queue message type
 ******************************************************************************/
typedef enum
{
    TOF_READ        = 1,   /* print distance to console (CLI) */
    TOF_READ_PACKET = 2    /* post distance to q_carpacket_report */
} tof_message_type_t;

/*******************************************************************************
 * Public interface
 ******************************************************************************/
extern QueueHandle_t q_tof;

void task_tof_init(void);

#endif /* __TASK_TOF_H__ */
