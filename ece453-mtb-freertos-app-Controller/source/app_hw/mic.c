// /**
//  * Samples the signal from the microphone at 8Khz and stores it in a buffer.
//  * test_with_bluetooth set to true is unfinished. Set to false plays it through dac off the same MCU.
//  */
// #include "mic.h"
// static cyhal_dac_t dac_obj;
// static BaseType_t cli_cmd_audio(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
// // initialize MOD_0_PIN_ADC_AIN as ADC input

// bool test_with_bluetooth = true; // SET true if using BT, false if using same MCU.

// // Audio Hardware Objects
// static cyhal_adc_t adc_obj;
// static cyhal_adc_channel_t adc_chan_obj;
// static cyhal_timer_t timer_obj;

// // Buffers
// static uint16_t audio_buffer[BUFFER_SIZE];
// static uint16_t buffer_index = 0;
// static volatile audio_state_t current_state = STATE_IDLE;

// // CLI Buffer
// // char cmd_buf[32];
// // uint8_t cmd_idx = 0;

// // Timer Interrupt Handler (8kHz)
// void isr_timer_8khz(void *callback_arg, cyhal_timer_event_t event)
// {
//     if (current_state == STATE_RECORDING)
//     {
//         if (buffer_index < BUFFER_SIZE)
//         {
//             // Read 12-bit ADC (scaled to 16-bit 0-65535 by HAL)
//             audio_buffer[buffer_index++] = cyhal_adc_read_u16(&adc_chan_obj);
//         }
//         else
//         {
//             current_state = STATE_IDLE;
//             cyhal_gpio_write(MOD_0_LOAD_SWITCH_1, 0); // Turn of microphone power.
//             // BT transmission of the audio_buffer sent here.

//         }
//     }
//     else if (current_state == STATE_PLAYBACK)
//     {
//         if (buffer_index < BUFFER_SIZE)
//         {
//             // Write to DAC
//             if (!test_with_bluetooth)
//             {
//                 cyhal_dac_write(&dac_obj, audio_buffer[buffer_index++]);
//             }
//             else
//             {
//                 // implement BT transmission of a logic 1 in the correct digit to play sound.
//             }
//         }
//         else
//         {
//             current_state = STATE_IDLE;
//         }
//     }
// }

// void audio_init(void)
// {
//     cy_rslt_t rslt;
//     // 1. Initialize Load pin
//     cyhal_gpio_init(MOD_0_LOAD_SWITCH_1, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
//     rslt = cyhal_adc_init(&adc_obj, MOD_0_PIN_ADC_AIN, NULL);
//     if (rslt != CY_RSLT_SUCCESS)
//     {
//         CY_ASSERT(0); // ADC Block failed to initialize
//     }

//     const cyhal_adc_config_t adc_config = {
//         .continuous_scanning = true,
//         .resolution = 12};

//     cyhal_adc_configure(&adc_obj, &adc_config);

//     const cyhal_adc_channel_config_t channel_config = {
//         .enable_averaging = false,
//         .min_acquisition_ns = 220,
//         .enabled = true};

//     rslt = cyhal_adc_channel_init_diff(&adc_chan_obj, &adc_obj, MOD_0_PIN_ADC_AIN, CYHAL_ADC_VNEG, &channel_config);
//     if (rslt != CY_RSLT_SUCCESS)
//     {
//         CY_ASSERT(0);
//     }

//     // 2. Initialize DAC
//     if (test_with_bluetooth == true)
//     {
//         // If using Bluetooth, skip
//         //snprintf(pcWriteBuffer, xWriteBufferLen, "Audio initialized for Bluetooth transmission. No local playback.\r\n");
//     }
//     else
//     {
//         rslt = cyhal_dac_init(&dac_obj, MOD_1_PIN_DAC);
//         if (rslt != CY_RSLT_SUCCESS)
//         {
//             CY_ASSERT(0);
//             printf("DAC init failed\r\n");
//         }
//         // Mid-scale bias
//         cyhal_dac_write(&dac_obj, DAC_MAX / 2);
//     }

//     // 3. Initialize Timer for 8kHz (125 microseconds)
//     const cyhal_timer_cfg_t timer_cfg = {
//         .is_continuous = true,
//         .direction = CYHAL_TIMER_DIR_UP,
//         .is_compare = false,
//         .period = 124,      // 100ms
//         .compare_value = 0, // Not used here
//         .value = 0,
//     };

//     cyhal_timer_init(&timer_obj, NC, NULL);
//     cyhal_timer_configure(&timer_obj, &timer_cfg);
//     cyhal_timer_set_frequency(&timer_obj, 1000000); // 1MHz clock
//     cyhal_timer_register_callback(&timer_obj, isr_timer_8khz, NULL);
//     cyhal_timer_enable_event(&timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, CYHAL_ISR_PRIORITY_DEFAULT, true);

//     // printf("PSoC 6 Audio Ready. Type 'record' to begin.\r\n");

//     // Register CLI command
//     static const CLI_Command_Definition_t audio_command = {
//         "audio", // The command string the user types
//         "\r\naudio <record|play>:\r\n  Starts recording or playing audio\r\n",
//         cli_cmd_audio, // The function above
//         1              // We now expect exactly 1 parameter
//     };

//     FreeRTOS_CLIRegisterCommand(&audio_command);
// }

// static BaseType_t cli_cmd_audio(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
// {
//     const char *param;
//     BaseType_t param_len;

//     // Get the first parameter after the command name
//     param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);

//     if (param == NULL)
//     {
//         snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Please specify 'on' or 'off'\r\n");
//     }
//     // Logic for "ON" (Starts recording/Mic Power)
//     else if (strncmp(param, "record", param_len) == 0)
//     {
//         cyhal_gpio_write(MOD_0_LOAD_SWITCH_1, 1);
//         cyhal_system_delay_us(50);
//         buffer_index = 0;
//         cyhal_timer_start(&timer_obj);
//         current_state = STATE_RECORDING;
//         snprintf(pcWriteBuffer, xWriteBufferLen, "Recording Started - Mic Powered\r\n");
//     }
//     // Logic for "OFF" (Stops recording/Powers down)
//     else if (strncmp(param, "play", param_len) == 0)
//     {
//         buffer_index = 0;
//         current_state = STATE_PLAYBACK;
//         snprintf(pcWriteBuffer, xWriteBufferLen, "Status: PLAYING...\r\n");
//     }
//     else
//     {
//         snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid parameter: %.*s\r\n", (int)param_len, param);
//     }

//     return pdFALSE;
// }