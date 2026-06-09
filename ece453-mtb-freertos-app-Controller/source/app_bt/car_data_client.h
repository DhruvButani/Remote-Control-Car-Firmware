/******************************************************************************
 * File Name: car_data_client.h
 *
 * Description: CarData BLE client - notification subscribe/parse and
 *              CarDataIn write helpers.
 ******************************************************************************/

#ifndef CAR_DATA_CLIENT_H_
#define CAR_DATA_CLIENT_H_

#include <stdint.h>
#include "wiced_bt_gatt.h"

/******************************************************************************
 *                          Parsed notification data
 ******************************************************************************/
typedef struct
{
    uint8_t speed;      // bits 5-0 (0-63)
    uint8_t light;      // bit 6
    uint8_t autobrake;  // bit 7
} car_data_out_t;

/******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/

/**
 * Subscribe to CarDataOut notifications by writing 0x0001 to its CCCD.
 *
 * @param conn_id           Active GATT connection id
 * @param cccd_handle       Handle of CarDataOut's Client Characteristic
 *                          Configuration Descriptor (discovered during
 *                          descriptor discovery, expected 0x000D)
 * @return GATT status
 */
wiced_bt_gatt_status_t car_data_subscribe_notifications(uint16_t conn_id,
                                                        uint16_t cccd_handle);

/**
 * Send a pre-packed events byte to the Car via CarDataIn.
 *
 * Byte format (one bit per Controller_Events field, record excluded):
 *   Bit 0 - horn
 *   Bit 1 - pair
 *   Bit 2 - reset
 *   Bit 3 - accelerate
 *   Bit 4 - brake
 *   Bit 5 - turn_left
 *   Bit 6 - turn_right
 *   Bit 7 - gear_sel
 *
 * @param conn_id           Active GATT connection id
 * @param val_handle        CarDataIn value handle (discovered, expected 0x000F)
 * @param events_byte       Packed events byte
 * @return GATT status
 */
wiced_bt_gatt_status_t car_data_send_control(uint16_t conn_id,
                                             uint16_t val_handle,
                                             uint8_t  events_byte);

/**
 * Parse a CarDataOut notification and print the result.
 *
 * @param p_data            GATT operation complete event from the stack
 * @param out_val_handle    CarDataOut value handle so we can verify the source
 * @param result            Parsed fields written here (may be NULL if you only
 *                          want the printf side-effect)
 */
void car_data_process_notification(wiced_bt_gatt_operation_complete_t *p_data,
                                   uint16_t out_val_handle,
                                   car_data_out_t *result);

#endif /* CAR_DATA_CLIENT_H_ */
