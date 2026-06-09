/**
 * @file task_acc.h
 * @brief
 * FreeRTOS task header for the LSM6DSM 6-axis IMU (accelerometer used).
 *
 * Other tasks (or the CLI) post an acc_message_type_t value to q_acc to
 * request a one-shot acceleration reading.  task_acc() reads the X/Y/Z
 * accelerometer channels, computes the total magnitude (mg), and either
 * prints it to the serial console or posts a 6-bit packed value to
 * q_carpacket_report for the car-status packet.
 */

#ifndef __TASK_ACC_H__
#define __TASK_ACC_H__

#include "main.h"
#include "ece453_pins.h"

/*******************************************************************************
 * Sensor / I2C configuration
 *   The LSM6DSM shares the Module-0 I2C bus with the LTR-390UV sensor.
 *   I2C 7-bit address = 0xD5 >> 1 = 0x6A (SA0 tied low — default).
 ******************************************************************************/
// #define ACC_I2C_ADDR                (0x6AU)
#define ACC_I2C_ADDR  (LSM6DSM_I2C_ADD_H >> 1U) 
/* Acceleration packing
 * --------------------
 * The CarPacket bits 5:0 carry a 6-bit code derived from total
 * acceleration magnitude (sqrt(x^2 + y^2 + z^2)) in milli-g:
 *
 *      acc6 = clip( |a|_mg / ACC_PACKET_MG_PER_LSB , 0..63 )
 *
 * With 32 mg per LSB the full 6-bit range covers 0..2016 mg, so a
 * stationary board (~1000 mg of gravity) sits near code 31.
 */
#define ACC_PACKET_MG_PER_LSB       (32U)
#define ACC_PACKET_MAX_CODE         (0x3FU)

/*******************************************************************************
 * Queue message type
 ******************************************************************************/
typedef enum
{
    ACC_READ        = 1,   /* print magnitude to console (CLI) */
    ACC_READ_PACKET = 2    /* post 6-bit code to q_carpacket_report */
} acc_message_type_t;

/*******************************************************************************
 * Public interface
 ******************************************************************************/
extern QueueHandle_t q_acc;

void task_acc_init(void);

#endif /* __TASK_ACC_H__ */
