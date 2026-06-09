/**
 * @file buttons.h
 * @author  Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include "main.h"
#include <stdio.h>
#include "cybsp.h"
#include "cyhal_gpio.h"
#include "cyhal_timer.h" 
#include "timer.h"

#define PIN_HORN P9_0
#define PIN_PAIR P9_1
#define PIN_RESET P9_2
#define PIN_ACCELERATE P9_3
#define PIN_BRAKE P9_4
#define PIN_SW_GEAR_SEL P9_5
#define PIN_JOY_BTTN_RECORD P9_6

#define PORT_ALL_BUTTONS GPIO_PRT9

#define MASK_BUTTON_HORN (1 << 0)
#define MASK_BUTTON_PAIR (1 << 1)
#define MASK_BUTTON_RESET (1 << 2)
#define MASK_TRIGGER_ACCELERATE (1 << 3)
#define MASK_TRIGGER_BRAKE (1 << 4)
#define MASK_SW_GEAR_SEL (1 << 5)
#define MASK_JOY_BUTTON (1 << 6)


/**
#include "FreeRTOS.h"
#include "task.h"
#include "task_console.h"
*/
typedef enum {
    BUTTON_SW1 = 0,
    BUTTON_SW2,
    BUTTON_SW3,
} ece453_button_t;

typedef enum {
    BUTTON_STATE_FALLING_EDGE = 0,
    BUTTON_STATE_LOW,
    BUTTON_STATE_HIGH,
    BUTTON_STATE_RISING_EDGE,
} button_state_t;

// high, going to high, or low, going to low
// switches are active low

// Function to initialize the buttons
cy_rslt_t buttons_init_gpio(void);

// Function to initialize the timer for button debouncing
//cy_rslt_t buttons_init_timer(void);

// Function to read the state of a specific button
//button_state_t buttons_get_state(ece353_button_t button);



#endif