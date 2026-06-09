/**
 * @file task_ltr390uv.h
 * @brief
 * FreeRTOS task header for the LTR-390UV-01 Ambient Light / UV sensor.
 *
 * Other tasks (or the CLI) post a ltr390uv_message_type_t value to
 * q_ltr390uv to request a one-shot ALS + UVS reading.  task_ltr390uv()
 * handles the request and prints lux and UVI to the serial console.
 */

#ifndef __TASK_LTR390UV_H__
#define __TASK_LTR390UV_H__

#include "main.h"
#include "ece453_pins.h"

/*******************************************************************************
 * Pin definitions
 *   Module 1 (P9_0 / P9_1) is already occupied by the VL53LX ToF sensor.
 *   LTR-390UV is wired to Module 0 connector (P10_0 / P10_1).
 *   If your PCB uses Module 2 instead, swap to MOD_2_PIN_I2C_SCL/SDA (P6_4/P6_5).
 ******************************************************************************/
#define LTR390UV_PIN_SCL        MOD_0_PIN_I2C_SCL   /* P10_0 */
#define LTR390UV_PIN_SDA        MOD_0_PIN_I2C_SDA   /* P10_1 */

/*******************************************************************************
 * Sensor / I2C configuration
 ******************************************************************************/
#define LTR390UV_I2C_ADDR           (0x53U)     /* 7-bit slave address        */
#define LTR390UV_I2C_SPEED_HZ       (400000U)   /* 400 kHz fast-mode          */
#define LTR390UV_CONVERSION_MS      (110U)      /* wait after mode change     */

/*******************************************************************************
 * Register map  (from datasheet §6)
 ******************************************************************************/
#define LTR390UV_REG_MAIN_CTRL      (0x00U)
#define LTR390UV_REG_MEAS_RATE      (0x04U)
#define LTR390UV_REG_GAIN           (0x05U)
#define LTR390UV_REG_PART_ID        (0x06U)
#define LTR390UV_REG_MAIN_STATUS    (0x07U)
#define LTR390UV_REG_ALS_DATA_0     (0x0DU)
#define LTR390UV_REG_ALS_DATA_1     (0x0EU)
#define LTR390UV_REG_ALS_DATA_2     (0x0FU)
#define LTR390UV_REG_UVS_DATA_0     (0x10U)

/*******************************************************************************
 * MAIN_STATUS bit masks  (from datasheet §6.5)
 ******************************************************************************/
#define LTR390UV_STATUS_DATA_READY  (0x08U)   /* Bit 3: new data available    */

/*******************************************************************************
 * Polling and threshold configuration
 ******************************************************************************/
#define LTR390UV_POLL_DELAY_MS          (10U)    /* ms between status polls    */

/* ALS lux threshold: readings above this are reported as "Light: ON".
 * At gain=18x, 18-bit/100ms: Lux ≈ 0.033 * ALS_DATA
 * A raw count of 500 corresponds to ~16 lux (dim indoor light).           */
#define LTR390UV_LUX_THRESHOLD_COUNTS   (500U)

/*******************************************************************************
 * MAIN_CTRL command values
 ******************************************************************************/
#define LTR390UV_CTRL_STANDBY       (0x00U)   /* sensor idle                  */
#define LTR390UV_CTRL_ALS_ACTIVE    (0x02U)   /* ALS mode, enabled            */
#define LTR390UV_CTRL_UVS_ACTIVE    (0x0AU)   /* UVS mode, enabled            */

/*******************************************************************************
 * Sensor settings used by this task
 *   Gain  = 18x  (register value 0x04)
 *   Res   = 18-bit, 100 ms conversion  (register value 0x22)
 *   WFAC  = 1.0  (no cover glass)
 *   UV sensitivity = 2300 counts / UVI  (datasheet §4.5)
 ******************************************************************************/
#define LTR390UV_GAIN_REG_VAL       (0x04U)   /* gain x18                     */
#define LTR390UV_MEAS_REG_VAL       (0x22U)   /* 18-bit, 100 ms               */
#define LTR390UV_GAIN_FACTOR        (18.0f)
#define LTR390UV_INT_FACTOR         (1.0f)    /* integration time factor @18b */
#define LTR390UV_UV_SENSITIVITY     (2300.0f)

/*******************************************************************************
 * Queue message type
 ******************************************************************************/
typedef enum
{
    LTR390UV_READ        = 1,  /* print ALS result to console (CLI) */
    LTR390UV_READ_PACKET = 2   /* post ALS count to q_carpacket_report */
} ltr390uv_message_type_t;

/*******************************************************************************
 * Public interface
 ******************************************************************************/
extern QueueHandle_t q_ltr390uv;

void task_ltr390uv_init(void);

#endif /* __TASK_LTR390UV_H__ */