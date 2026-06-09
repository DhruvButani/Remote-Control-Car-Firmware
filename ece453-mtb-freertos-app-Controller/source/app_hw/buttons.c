/**
 * @file buttons.c
 * @author 
 * @brief
 * @version 0.1
 * @date 2025-06-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "buttons.h"

static cyhal_timer_t button_timer;
static cyhal_timer_cfg_t button_timer_cfg;


static void button_timer_handler(void *arg, cyhal_timer_event_t event)
{
    static uint8_t button_counts[7] = {0, 0, 0,0,0};

    uint8_t BUTTONS = PORT_ALL_BUTTONS->IN ;

    uint8_t HORN = BUTTONS & MASK_BUTTON_HORN;
    uint8_t PAIR = BUTTONS & MASK_BUTTON_PAIR;
    uint8_t RESET = BUTTONS & MASK_BUTTON_RESET;
    uint8_t ACCELERATE = BUTTONS & MASK_TRIGGER_ACCELERATE;
    uint8_t BRAKE = BUTTONS & MASK_TRIGGER_BRAKE;
    uint8_t SW_GEAR_SEL = BUTTONS & MASK_SW_GEAR_SEL;
    uint8_t JOY_BTTN_RECORD = BUTTONS & MASK_JOY_BUTTON;

    if (HORN != 0) // active high
    {
        button_counts[0]++;

        if (button_counts[0] == 5)
        {
            Controller_Events.horn = 1;
        }
    }
    else
    {
        button_counts[0] = 0;
    }
    if (PAIR != 0)
    {
        button_counts[1]++;

        if (button_counts[1] == 5)
        {
            Controller_Events.pair = 1;
        }
    }
    else
    {
        button_counts[1] = 0;
    }
    if (RESET != 0)
    {
        button_counts[2]++;

        if (button_counts[2] == 5)
        {
            Controller_Events.reset = 1;
        }
    }
    else
    {
        button_counts[2] = 0;
    }
    if (ACCELERATE != 0)
    {
        button_counts[3]++;

        if (button_counts[3] == 5)
        {
            Controller_Events.accelerate = 1;
        }
    }
    else
    {
        button_counts[3] = 0;
        Controller_Events.accelerate = 0;
    }
    if (BRAKE != 0)
    {
        button_counts[4]++;

        if (button_counts[4] == 5)
        {
            Controller_Events.brake = 1;
        }
    }
    else
    {
        button_counts[4] = 0;
        Controller_Events.brake = 0;
    }
    if (SW_GEAR_SEL != 0)
    {
        button_counts[5]++;

        if (button_counts[5] == 5)
        {
            Controller_Events.gear_sel = 0;
        }
    }
    else
    {
        button_counts[5] = 0;
        Controller_Events.gear_sel = 1;
    }
    if (JOY_BTTN_RECORD != 0)
    {
        button_counts[6]++;

        if (button_counts[6] == 5)
        {
            Controller_Events.record = 1;
        }
    }
    else
    {
        button_counts[6] = 0;
    }

}

cy_rslt_t buttons_init_gpio(void)
{

    cy_rslt_t result;

    result = cyhal_gpio_init(PIN_HORN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Horn button GPIO\n");
        return result;
    }
    result = cyhal_gpio_init(PIN_PAIR, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Pair button GPIO\n");
        return result;
    }

    result = cyhal_gpio_init(PIN_RESET, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Reset button GPIO\n");
        return result;
    }

    result = cyhal_gpio_init(PIN_ACCELERATE, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Accelerate button GPIO\n");
        return result;
    }

    result = cyhal_gpio_init(PIN_BRAKE, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Brake button GPIO\n");
        return result;
    }

    result = cyhal_gpio_init(PIN_SW_GEAR_SEL, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Gear Selection button GPIO\n");
        return result;
    }

    result = cyhal_gpio_init(PIN_JOY_BTTN_RECORD, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    if (result != CY_RSLT_SUCCESS)
    {
        //printf("Failed to initialize Record button GPIO\n");
        return result;
    }

    timer_init(&button_timer, &button_timer_cfg, 500000, button_timer_handler);

    return result;
}


/* Function to initialize the timer for button debouncing
cy_rslt_t buttons_init_timer(void)
{
    // Initialize the timer for button debouncing for a period of 5ms
    return timer_init(&button_timer, &button_timer_cfg, 500000, button_timer_handler);
}
*/