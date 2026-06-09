/**
 * @file task_sendCarPacket.h
 * @brief
 * FreeRTOS task that assembles and transmits the 1-byte Car -> Controller
 * status packet every CARPACKET_PERIOD_MS.
 *
 * Packet layout (Server -> Client via CarDataOut notification):
 *   Bit 7    : UV/light sensor state    (1 = above LTR390UV threshold)
 *   Bit 6    : Autobrake                (1 = ToF distance < 400 mm)
 *   Bits 5:0 : Reserved (filled with 1s; accelerometer unused)
 *
 * The task requests fresh readings from each hardware task through their
 * existing request queues, then blocks on q_carpacket_report for their
 * responses.  It then prints the assembled byte to the serial console and
 * pushes it into the GATT CarDataOut characteristic so the BLE stack can
 * notify the connected client.
 */

#ifndef __TASK_SEND_CAR_PACKET_H__
#define __TASK_SEND_CAR_PACKET_H__

#include "main.h"

/*******************************************************************************
 * Configuration
 ******************************************************************************/
#define CARPACKET_PERIOD_MS         (500U)
#define CARPACKET_AUTOBRAKE_MM      (400U)

/*******************************************************************************
 * Cross-task report message
 ******************************************************************************/
typedef enum
{
    PKT_SENSOR_LTR390UV = 0,
    PKT_SENSOR_TOF      = 1,
    PKT_SENSOR_ACC      = 2,
} packet_sensor_id_t;

typedef struct
{
    packet_sensor_id_t sensor_id;
    uint32_t           value;   /* ALS count, distance in mm, or 6-bit acc code */
} packet_sensor_report_t;

/*******************************************************************************
 * Public interface
 ******************************************************************************/
extern QueueHandle_t q_carpacket_report;

void task_sendCarPacket_init(void);

#endif /* __TASK_SEND_CAR_PACKET_H__ */
