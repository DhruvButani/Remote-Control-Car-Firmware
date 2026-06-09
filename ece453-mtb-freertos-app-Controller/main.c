/**
 * @file main.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-05-14
 * 
 * @copyright Copyright (c) 2024
 * 
 * This program utilizes xbuttons, joystick, microphone, and an LCD for the RC car controller. 
 * Buttons: all buttons are monitored on a timer and their status is monitored in an event.
 * Joystick: when the joystick has a new position, event is updated.
 * When new speed status is recieved from the car, the LCD is updated.
 * 
 */
#include "main.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_blink.h"
#include "source/app_hw/mic.h"
#include "source/app_hw/buttons.h"
#include "source/app_hw/joystick.h"
#include "app_hw/task_lcd.h"
//#include "controller_events.h"



// BLE header files
#include "app_flash_common.h"
#include "wiced_bt_stack.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <string.h>
#include "cybt_platform_trace.h"
#include "wiced_memory.h"
#include "cyhal.h"
#include "stdio.h"
#include "GeneratedSource/cycfg_bt_settings.h"
#include "GeneratedSource/cycfg_gap.h"
#include "wiced_bt_dev.h"
#include "app_bt_utils.h"
#include "battery_client.h"
#include "mtb_kvstore.h"
#include "cybt_platform_config.h"
#include "cybsp_bt_config.h"
#include "app_bt_bonding.h"



/*******************************************************************************
*        BLE Macro Definitions
*******************************************************************************/
/* Stack size */
#define BT_STACK_HEAP_SIZE                  (1024 * 6)

/* FreeRTOS Task Configurations for Battery Client BTN TASK */

/* Stack size for Battery Client BTN task */
#define BTN_TASK_STACK_SIZE                 (512u)

/* Task Priority of Battery CLient BTN Task */
#define BTN_TASK_PRIORITY                   (2)

/*******************************************************************************
* BLE Variable Definitions
*******************************************************************************/
extern TaskHandle_t button_handle;

/*******************************************************************************
* Controller Variable Definitions
*******************************************************************************/
volatile controller_events_t Controller_Events;

int main(void)
{ 
    
    cy_rslt_t rslt;
    wiced_result_t wiced_result;

    /* Initialize the device and board peripherals */
    rslt = cybsp_init() ;
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    __enable_irq();

                                                /***** CONSOLE INIT (must be before BLE so task_print works) *****/
    // TEMP: disabled to free SCB5 (P5_0/P5_1) so LCD SPI can claim it.
    // task_console_init() internally brings up the UART on SCB5 AND creates
    // the Console Tx/Rx FreeRTOS tasks, so commenting this single call is
    // sufficient to release SCB5 and stop both tasks.
    task_console_init();

                                                /***** BLE INITIALIZATIONS *****/

    /* Configure platform specific settings for the BT device */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /*Initialize the block device used by kv-store for performing read/write operations to the flash*/
    app_kvstore_bd_config(&block_device);

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(battery_client_management_callback, &wiced_bt_cfg_settings);

    /* Button handling for BLE connection */
    xTaskCreate(button_task, "Button task", BTN_TASK_STACK_SIZE, NULL, BTN_TASK_PRIORITY, &button_handle); 

    /* Check if stack initialization was successful */
    if( WICED_BT_SUCCESS == wiced_result) {
        task_print("Bluetooth Stack Initialization Successful \n"); 
    }
    else {
        task_print("Bluetooth Stack Initialization failed!! \n");
        CY_ASSERT(0);
    }
    if (wiced_bt_create_heap("default_heap", NULL, BT_STACK_HEAP_SIZE, NULL, WICED_TRUE) == NULL) {
        task_print("create default heap error: size %d\n", BT_STACK_HEAP_SIZE);
    }
    else {
        task_print("create default heap sucessful: size %d\n", BT_STACK_HEAP_SIZE);
    }

                                                /***** APPLICATION TASK INITIALIZATIONS *****/

    // *Do not initlize HW here, do it in battery_client.c*

    // buttons_init_gpio(); // wont work right now since DAC uses P9_6
    status_cli_init();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();
    
    for (;;)
    {
    }
}

/* [] END OF FILE */
