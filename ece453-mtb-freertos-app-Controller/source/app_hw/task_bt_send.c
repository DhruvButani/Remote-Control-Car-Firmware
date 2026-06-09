#include "task_bt_send.h"
#include "timer.h"
#include "controller_events.h"
#include "car_data_client.h"
#include "battery_client.h"

#include "FreeRTOS.h"
#include "task.h"

/* 100 MHz timer clock → 20,000,000 ticks == 200 ms */
#define BT_SEND_PERIOD_TICKS    (20000000U)

#define BT_SEND_TASK_STACK_WORDS   (configMINIMAL_STACK_SIZE * 2)
#define BT_SEND_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)

static cyhal_timer_t     bt_send_timer;
static cyhal_timer_cfg_t bt_send_timer_cfg;
static TaskHandle_t      bt_send_task_handle = NULL;

static void bt_send_timer_isr(void *arg, cyhal_timer_event_t event)
{
    (void)arg;
    (void)event;

    if (bt_send_task_handle == NULL)
    {
        return;
    }

    BaseType_t higher_prio_woken = pdFALSE;
    vTaskNotifyGiveFromISR(bt_send_task_handle, &higher_prio_woken);
    portYIELD_FROM_ISR(higher_prio_woken);
}

static void bt_send_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        /* Block until the timer ISR notifies us. */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t conn_id    = battery_client_app_data.conn_id;
        uint16_t val_handle = car_data_in_get_val_handle();

        if (conn_id == 0 || val_handle == 0)
        {
            continue;
        }

        /* Pack one bit per Controller_Events field (record excluded).
         * Brake is hardcoded low for now. */
        uint8_t events_byte =
              ((Controller_Events.horn       & 0x01) << 0)
            | ((Controller_Events.pair       & 0x01) << 1)
            | ((Controller_Events.reset      & 0x01) << 2)
            | ((Controller_Events.accelerate & 0x01) << 3)
            | ((0u                           & 0x01) << 4)   /* brake */
            | ((Controller_Events.turn_left  & 0x01) << 5)
            | ((Controller_Events.turn_right & 0x01) << 6)
            | ((Controller_Events.gear_sel   & 0x01) << 7);

        car_data_send_control(conn_id, val_handle, events_byte);

        /* Clear edge-triggered flags after they've been transmitted so each
         * press fires for exactly one packet. */
        Controller_Events.horn  = 0;
        Controller_Events.pair  = 0;
        Controller_Events.reset = 0;
    }
}

cy_rslt_t task_bt_send_init(void)
{
    if (bt_send_task_handle != NULL)
    {
        /* Already initialized; do not create a second task/timer. */
        return CY_RSLT_SUCCESS;
    }

    BaseType_t task_ok = xTaskCreate(bt_send_task,
                                     "BtSend",
                                     BT_SEND_TASK_STACK_WORDS,
                                     NULL,
                                     BT_SEND_TASK_PRIORITY,
                                     &bt_send_task_handle);
    if (task_ok != pdPASS)
    {
        bt_send_task_handle = NULL;
        return (cy_rslt_t)CY_RSLT_TYPE_ERROR;
    }

    return timer_init(&bt_send_timer,
                      &bt_send_timer_cfg,
                      BT_SEND_PERIOD_TICKS,
                      bt_send_timer_isr);
}
