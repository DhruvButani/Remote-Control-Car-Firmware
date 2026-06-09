/*
 * adc.h
 */

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "main.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_result.h"
#include "cyhal_adc.h"
#include "timer.h"

#include <ctype.h>
#include <stdio.h>

#define JOYSTICK_ADC_RESOLUTION 12
#define JOYSTICK_ADC_MAX_VALUE  ((1 << JOYSTICK_ADC_RESOLUTION) - 1) // 4095 for 12-bit resolution

#define PIN_ANALOG_JOY_X P10_3
#define PIN_ANALOG_JOY_Y P10_4

#define JOYSTICK_THRESH_X_LEFT_2P475V   0xBFFF
#define JOYSTICK_THRESH_X_RIGHT_0P825V  0x3FFF
#define JOYSTICK_THRESH_Y_UP_2P475V     0xBFFF
#define JOYSTICK_THRESH_Y_DOWN_0P825V   0x3FFF

#define JOYSTICK_THRESH_X_LEFT   JOYSTICK_THRESH_X_LEFT_2P475V 
#define JOYSTICK_THRESH_X_RIGHT  JOYSTICK_THRESH_X_RIGHT_0P825V
#define JOYSTICK_THRESH_Y_UP     JOYSTICK_THRESH_Y_UP_2P475V   
#define JOYSTICK_THRESH_Y_DOWN   JOYSTICK_THRESH_Y_DOWN_0P825V 


/* Custom Data Typtes */
typedef enum {
    JOYSTICK_POS_CENTER,
    JOYSTICK_POS_LEFT,
    JOYSTICK_POS_RIGHT,
    JOYSTICK_POS_UP,
    JOYSTICK_POS_DOWN,
    JOYSTICK_POS_UPPER_LEFT,
    JOYSTICK_POS_UPPER_RIGHT,
    JOYSTICK_POS_LOWER_LEFT,
    JOYSTICK_POS_LOWER_RIGHT
}joystick_position_t;

/* Public Global Variables */

/* Public API */

/** Initialize the ADC channels connected to the Joystick
 *
 * @param - None
 */
cy_rslt_t joystick_init(void);

/** Read X direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_x(void);

/** Read Y direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_y(void);

/**
 * @brief 
 * Returns the current position of the joystick 
 * @return joystick_position_t 
 */
joystick_position_t joystick_get_pos(void);

/**
 * @brief 
 * Prints out a string to the console based on parameter passed 
 * @param position 
 * The enum value to be printed.
 */
void joystick_print_pos(joystick_position_t position);

#endif /* __JOYSTICK_H__ */
