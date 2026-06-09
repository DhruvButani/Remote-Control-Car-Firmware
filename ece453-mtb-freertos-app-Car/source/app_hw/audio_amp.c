/**
 * 0x03 — Control Register
 * Controls:
 * Gain (20–36 dB)
 * SpeakerGuard voltage limit
 * Switching frequency (400/500 kHz)
 *
 * 0x01 — Fault Register
 * Tells you if hardware protections triggered:
 * Overtemperature
 * Overcurrent
 * Undervoltage
 * Overvoltage
 * DC offset
 *
 * 0x02 — Status / Load Diagnostics
 * Tells you:
 * Play or mute mode
 * Load diagnostics state
 * Short to ground
 * Short to PVDD
 * Open load
 * Shorted load
 */
#include "audio_amp.h"
static cyhal_dac_t dac_obj;
static uint16_t sine_table[TABLE_SIZE];
static volatile bool tone_enable = false;

/************************************************************
 *                FORWARD DECLARATIONS
 *************************************************************/
static void task_play_tone(void *arg);
static void generate_sine_table(void);

static BaseType_t cli_cmd_tone(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/************************************************************
 *                    INIT FUNCTION
 *************************************************************/
void task_amp_init(void)
{
    cy_rslt_t rslt;

    /* Initialize DAC */
    
    rslt = cyhal_dac_init(&dac_obj, PIN_DAC);
    if (rslt != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
        printf("DAC init failed\r\n");
    }
    //Mid-scale bias
    cyhal_dac_write(&dac_obj, DAC_MAX / 2);

    generate_sine_table();

    //Create tone task
    xTaskCreate(
        task_play_tone,
        "Tone",
        512,
        NULL,
        1,
        NULL);

    //Register CLI command
    static const CLI_Command_Definition_t cmd_tone =
        {
            "tone",
            "\r\ntone <on|off>:\r\n Turns tone on or off\r\n",
            cli_cmd_tone,
            1};

    FreeRTOS_CLIRegisterCommand(&cmd_tone);
    
}

/************************************************************
 *               SINE TABLE GENERATION
 *************************************************************/
static void generate_sine_table(void)
{
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / TABLE_SIZE;
        float s = sinf(angle);

        float scaled = s * TONE_AMPLITUDE;

        sine_table[i] =
            (uint16_t)((scaled + 1.0f) * (DAC_MAX / 2));
    }
}

/************************************************************
 *                   TONE TASK
 *************************************************************/
static void task_play_tone(void *arg)
{
    (void)arg;
    uint32_t index = 0;

    while (1)
    {
        if (tone_enable)
        {

            cyhal_dac_write(&dac_obj, sine_table[index]);

            index++;
            if (index >= TABLE_SIZE)
                index = 0;

            cyhal_dac_write(&dac_obj, sine_table[index]);
            index++;
            if (index >= TABLE_SIZE)
                index = 0;
        }

        // delay 1khz
        cyhal_system_delay_us(21);
    }
}

/************************************************************
 *                   HORN CONTROL
 *************************************************************/
void audio_horn_on(void)
{
    tone_enable = true;
}

void audio_horn_off(void)
{
    tone_enable = false;
}

/************************************************************
 *                   CLI COMMAND
 *************************************************************/
static BaseType_t cli_cmd_tone(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *param;
    BaseType_t param_len;

    param = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &param_len);

    if (strncmp(param, "on", param_len) == 0)
    {
        tone_enable = true;
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Tone ON\r\n");
    }
    else if (strncmp(param, "off", param_len) == 0)
    {
        tone_enable = false;
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Tone OFF\r\n");
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Invalid parameter\r\n");
    }

    return pdFALSE;
}