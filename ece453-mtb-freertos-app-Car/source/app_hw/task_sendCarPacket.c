#include "task_sendCarPacket.h"
#include "task_console.h"
#include "task_ltr390uv.h"
#include "task_tof.h"
#include "task_acc.h"
#include "task_dc_motor.h"

#include "cycfg_gatt_db.h"
#include "app_bt_gatt_handler.h"

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

QueueHandle_t q_carpacket_report;

/******************************************************************************/
/* Static Task Definition                                                     */
/******************************************************************************/

/**
 * @brief Request a fresh reading from both sensor tasks, collect their
 *        reports via q_carpacket_report, then assemble and dispatch the
 *        1-byte status packet.
 */
static void task_sendCarPacket(void *param)
{
    ltr390uv_message_type_t ltr_msg = LTR390UV_READ_PACKET;
    tof_message_type_t      tof_msg = TOF_READ_PACKET;
    acc_message_type_t      acc_msg = ACC_READ_PACKET;
    packet_sensor_report_t  report;

    TickType_t last_wake = xTaskGetTickCount();

    uint32_t last_als_count = 0;
    uint32_t last_distance_mm = 0;
    uint32_t last_acc_code = 0;

    (void)param;

    for (;;)
    {
        /* Fire off a read request to each sensor task.  Non-blocking sends
         * — if a queue is already full (prior request still pending) we
         * simply reuse the most recent cached value below.            */
        (void)xQueueSendToBack(q_ltr390uv, &ltr_msg, 0);
        (void)xQueueSendToBack(q_tof,      &tof_msg, 0);
        (void)xQueueSendToBack(q_acc,      &acc_msg, 0);

        /* Collect reports, bounded by the period so we never
         * miss a tick if one sensor stalls.                           */
        uint8_t  got_ltr = 0, got_acc = 0;
        TickType_t deadline = xTaskGetTickCount() +
                              pdMS_TO_TICKS(CARPACKET_PERIOD_MS - 50);

        while (!(got_ltr && got_acc))
        {
            TickType_t now = xTaskGetTickCount();
            TickType_t wait = (deadline > now) ? (deadline - now) : 0;

            if (xQueueReceive(q_carpacket_report, &report, wait) != pdTRUE)
                break;

            switch (report.sensor_id)
            {
                case PKT_SENSOR_LTR390UV:
                    last_als_count = report.value;
                    got_ltr = 1;
                    break;
                case PKT_SENSOR_TOF:
                    last_distance_mm = report.value;
                    break;
                case PKT_SENSOR_ACC:
                    last_acc_code = report.value;
                    got_acc = 1;
                    break;
                default:
                    break;
            }
        }

        uint8_t uv_on       = (last_als_count   > LTR390UV_LUX_THRESHOLD_COUNTS) ? 1 : 0;
        uint8_t autobrake   = (last_distance_mm < CARPACKET_AUTOBRAKE_MM)        ? 1 : 0;
        uint8_t acc6        = (uint8_t)(last_acc_code & 0x3FU);

        if (autobrake)
        {
            (void)task_dc_motor_stop();
        }

        cyhal_gpio_write(P5_6, !uv_on);

        uint8_t packet = acc6;                    /* bits 5:0 = acc magnitude */
        packet |= (uint8_t)((uv_on     & 0x01U) << 7);
        packet |= (uint8_t)((autobrake & 0x01U) << 6);

        task_print_info(
            "CarPacket: 0x%02X [UV=%u ALS=%lu | Brake=%u Dist=%lu mm | Acc=0x%02X]\r\n",
            packet,
            uv_on,     (unsigned long)last_als_count,
            autobrake, (unsigned long)last_distance_mm,
            acc6);

        /* Publish to the GATT characteristic.  app_bt_send_message() is a
         * no-op when no client is subscribed (CCCD == 0), so it's safe to
         * call unconditionally.                                        */
        app_cardata_cardataout[0] = packet;
        app_bt_send_message();

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CARPACKET_PERIOD_MS));
    }
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

void task_sendCarPacket_init(void)
{
    /* Low-UV indicator: drives P5_6 high when ALS counts fall below threshold. */
    cyhal_gpio_init(P5_6, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

    /* Small queue — at most one pending report per sensor per cycle. */
    q_carpacket_report = xQueueCreate(4, sizeof(packet_sensor_report_t));
    configASSERT(q_carpacket_report != NULL);

    xTaskCreate(task_sendCarPacket,
                "Task_CarPacket",
                configMINIMAL_STACK_SIZE * 2,
                NULL,
                configMAX_PRIORITIES - 5,
                NULL);
}
