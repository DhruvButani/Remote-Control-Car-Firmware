/******************************************************************************
 * File Name: car_data_client.c
 *
 * Description: CarData BLE client - notification subscribe/parse and
 *              CarDataIn write helpers.
 ******************************************************************************/

#include "wiced_bt_stack.h"
#include "wiced_bt_gatt.h"
#include "car_data_client.h"
#include <stdio.h>
#include <string.h>
#include "task_console.h"

/*******************************************************************************
 * Function Name: car_data_subscribe_notifications
 *******************************************************************************
 * Summary:
 *   Write 0x0001 (enable notifications) to the CarDataOut CCCD handle.
 *   Uses the existing WICED utility so we don't have to build the write
 *   buffer by hand.
 ******************************************************************************/
wiced_bt_gatt_status_t car_data_subscribe_notifications(uint16_t conn_id,
                                                        uint16_t cccd_handle)
{
    wiced_bt_gatt_status_t status;

    if (cccd_handle == 0)
    {
        task_print("car_data_subscribe_notifications: CCCD handle is 0 — "
               "descriptor discovery may not have completed\n");
        return WICED_BT_GATT_WRONG_STATE;
    }

    task_print("Subscribing to CarDataOut notifications (CCCD handle: 0x%04X)\n",
           cccd_handle);

    uint8_t buf[2] = { GATT_CLIENT_CONFIG_NOTIFICATION & 0xFF,
                        (GATT_CLIENT_CONFIG_NOTIFICATION >> 8) & 0xFF };
    wiced_bt_gatt_write_hdr_t write_hdr;
    write_hdr.handle   = cccd_handle;
    write_hdr.offset   = 0;
    write_hdr.len      = 2;
    write_hdr.auth_req = 0;

    status = wiced_bt_gatt_client_send_write(conn_id,
                                             GATT_REQ_WRITE,
                                             &write_hdr,
                                             buf,
                                             NULL);

    task_print("Subscribe status: %d\n", status);
    return status;
}

/*******************************************************************************
 * Function Name: car_data_send_control
 *******************************************************************************
 * Summary:
 *   Write a pre-packed CarDataIn events byte to the server
 *   (Write Without Response for low latency). See header for bit layout.
 ******************************************************************************/
wiced_bt_gatt_status_t car_data_send_control(uint16_t conn_id,
                                             uint16_t val_handle,
                                             uint8_t  events_byte)
{
    wiced_bt_gatt_write_hdr_t write_hdr;

    if (val_handle == 0)
    {
        task_print("car_data_send_control: val_handle is 0 — "
               "characteristic discovery may not have completed\n");
        return WICED_BT_GATT_WRONG_STATE;
    }

    write_hdr.handle   = val_handle;
    write_hdr.offset   = 0;
    write_hdr.len      = 1;
    write_hdr.auth_req = 0;

    return wiced_bt_gatt_client_send_write(conn_id,
                                           GATT_CMD_WRITE,   /* Write Without Response */
                                           &write_hdr,
                                           &events_byte,
                                           NULL);
}

/*******************************************************************************
 * Function Name: car_data_process_notification
 *******************************************************************************
 * Summary:
 *   Called from the GATT notification handler.  Checks that the handle
 *   matches CarDataOut, unpacks the byte, prints it, and optionally fills
 *   in the caller-supplied struct.
 ******************************************************************************/
void car_data_process_notification(wiced_bt_gatt_operation_complete_t *p_data,
                                   uint16_t out_val_handle,
                                   car_data_out_t *result)
{
    uint8_t val;
    car_data_out_t parsed;

    if (p_data->response_data.att_value.handle != out_val_handle)
    {
        /* Not our characteristic — ignore silently */
        return;
    }

    if (p_data->response_data.att_value.len < 1)
    {
        task_print("CarDataOut notification with 0-length payload\n");
        return;
    }

    val = p_data->response_data.att_value.p_data[0];

    parsed.speed     = val & 0x3F;         /* bits 5-0 */
    parsed.light     = (val >> 6) & 0x01;  /* bit 6    */
    parsed.autobrake = (val >> 7) & 0x01;  /* bit 7    */

    task_print("CarDataOut -> speed:%d  light:%d  autobrake:%d\n",
           parsed.speed, parsed.light, parsed.autobrake);

    if (result != NULL)
    {
        *result = parsed;
    }
}
