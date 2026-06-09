/**
 * @file main.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-05-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "main.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_blink.h"
#include "source/app_hw/task_tof.h"
#include "source/app_hw/task_ltr390uv.h"
#include "source/app_hw/task_sendCarPacket.h"
#include "source/app_hw/i2c.h"


// BLE related includes
#include "cycfg_bt_settings.h"
#include "wiced_bt_stack.h"
#include "cybsp_bt_config.h"
#include "cybt_platform_config.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"
#include "app_bt_utils.h"
#include "app_flash_common.h"
#include "app_bt_bonding.h"


int main(void)
{
    cy_rslt_t rslt;
    wiced_result_t wiced_result;

    /* Initialize the device and board peripherals */
    rslt = cybsp_init() ;
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    __enable_irq();

    task_console_init(); //so task_print works


                            //BLE initialization 
    /* Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /*Initialize the block device used by kv-store for performing
     * read/write operations to the flash*/
    app_kvstore_bd_config(&block_device);

    /* Register call back and configuration with stack */

    wiced_result = wiced_bt_stack_init(app_bt_management_callback, &wiced_bt_cfg_settings);

    /* Check if stack initialization was successful */
    if(WICED_BT_SUCCESS == wiced_result)
    {
        task_print_info("Bluetooth Stack Initialization Successful \n"); //?
    }
    else
    {
        task_print_info("Bluetooth Stack Initialization failed!! \n");
        CY_ASSERT(0);
    }
    
    //Task initialization



    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();
    
    for (;;)
    {
    }
}

/* [] END OF FILE */