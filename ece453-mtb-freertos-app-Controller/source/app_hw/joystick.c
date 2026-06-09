#include "joystick.h"
#include "cy_result.h"
#include "cyhal_adc.h"

/**
 * Add a timer to periodically read the joystick position and update the controller events struct.
 * 
 */

/* Static Global Variables */

static cyhal_timer_t joy_timer;
static cyhal_timer_cfg_t joy_timer_cfg;


static cyhal_adc_t joystick_adc_obj;
static cyhal_adc_channel_t joystick_adc_chan_x_obj;
static cyhal_adc_channel_t joystick_adc_chan_y_obj;
const cyhal_adc_channel_config_t channel_config = { .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };

const cyhal_adc_config_t joystick_config = {
    .continuous_scanning = true, 
    .resolution = 12, 
    .average_count = 1, 
    .average_mode_flags = 0,
    .ext_vref_mv = 0,
    .vneg = CYHAL_ADC_VNEG_VREF,
    .vref = CYHAL_ADC_REF_VDDA_DIV_2,
    .ext_vref = NC,
    .is_bypassed = false,
    .bypass_pin = NC
};

/* Public API */

static void joy_timer_handler(void *arg, cyhal_timer_event_t event)
{
    joystick_position_t current_pos = joystick_get_pos();
    if (current_pos == JOYSTICK_POS_LEFT || current_pos == JOYSTICK_POS_UPPER_LEFT || current_pos == JOYSTICK_POS_LOWER_LEFT)
    {
        Controller_Events.turn_left = 1;
    }
    else
    {
        Controller_Events.turn_left = 0;
    }

    if (current_pos == JOYSTICK_POS_RIGHT || current_pos == JOYSTICK_POS_UPPER_RIGHT || current_pos == JOYSTICK_POS_LOWER_RIGHT)
    {
        Controller_Events.turn_right = 1;
    }
    else
    {
        Controller_Events.turn_right = 0;
    }
}

/** Initialize the ADC channels connected to the Joystick
 *
 * @param - None
 */
cy_rslt_t joystick_init(void)
{
    cy_rslt_t rslt;

    /* Initialize ADC.*/
    rslt = cyhal_adc_init(&joystick_adc_obj, PIN_ANALOG_JOY_X, NULL);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Set the Reference Voltage to be VDDA */
    rslt = cyhal_adc_configure(&joystick_adc_obj, &joystick_config);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }


    /* Initialize X direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_x_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_X, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Initialize Y direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_y_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_Y, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );

    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    timer_init(&joy_timer, &joy_timer_cfg, 500000, joy_timer_handler);
    return rslt;
}

/** Read X direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_x(void)
{
    /* ADD CODE */
    return cyhal_adc_read_u16(&joystick_adc_chan_x_obj);

}

/** Read Y direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_y(void)
{
    /* ADD CODE */
    return cyhal_adc_read_u16(&joystick_adc_chan_y_obj);
}


/**
 * @brief 
 * Returns the current position of the joystick 
 * @return joystick_position_t 
 */
joystick_position_t joystick_get_pos(void)
{
    uint16_t x_val;
    uint16_t y_val;
    joystick_position_t position = JOYSTICK_POS_CENTER;

    x_val = joystick_read_x();
    y_val = joystick_read_y();

    if(x_val < JOYSTICK_THRESH_X_LEFT_2P475V && x_val > JOYSTICK_THRESH_X_RIGHT_0P825V && 
        y_val < JOYSTICK_THRESH_Y_UP && y_val > JOYSTICK_THRESH_Y_DOWN){
        return 0; // center
    }
    if(x_val > JOYSTICK_THRESH_X_LEFT_2P475V && 
        y_val < JOYSTICK_THRESH_Y_UP && y_val > JOYSTICK_THRESH_Y_DOWN){
        return 1; // left
    }
    if(x_val < JOYSTICK_THRESH_X_RIGHT &&
        y_val > JOYSTICK_THRESH_Y_DOWN && y_val < JOYSTICK_THRESH_Y_UP){
        return 2; // right
    }
    if(x_val < JOYSTICK_THRESH_X_LEFT_2P475V && x_val > JOYSTICK_THRESH_X_RIGHT_0P825V && y_val > JOYSTICK_THRESH_Y_UP){
        return 3; // Up
    }
    if(x_val < JOYSTICK_THRESH_X_LEFT_2P475V && x_val > JOYSTICK_THRESH_X_RIGHT_0P825V && y_val < JOYSTICK_THRESH_Y_DOWN){
        return 4; // Down
    }
    if(x_val > JOYSTICK_THRESH_X_LEFT_2P475V && y_val > JOYSTICK_THRESH_Y_UP){
        return 5; // upper left
    }
    if(x_val < JOYSTICK_THRESH_X_RIGHT_0P825V && y_val > JOYSTICK_THRESH_Y_UP){
        return 6; // Upper right
    }
    if(x_val > JOYSTICK_THRESH_X_LEFT_2P475V && y_val < JOYSTICK_THRESH_Y_DOWN){
        return 7; // lower left
    }
    if(x_val < JOYSTICK_THRESH_X_RIGHT_0P825V && y_val < JOYSTICK_THRESH_Y_DOWN){
        return 8; // lower right
    }

    
    /* ADD CODE */
    return position;
}
