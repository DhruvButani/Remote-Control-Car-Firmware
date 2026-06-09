#ifndef __TASK_AUDIO_AMP_H__
#define __TASK_AUDIO_AMP_H__

#include "main.h"
#include "task_console.h"
#include "i2c.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cy_result.h"
#include <math.h>
#include "cyhal_dac.h"

#define SAMPLE_RATE 48000
#define TONE_FREQ   1000
#define TABLE_SIZE  48
#define TONE_AMPLITUDE 0.3f
#define DAC_MAX ((1 << 16) - 1)

#define PIN_DAC		P9_6

#define ADDR_WRITE  0xD8
#define ADDR_READ   0xD9
#define ADDR_SUBORDINATE (ADDR_WRITE >> 1)
#define ADDR_FAULT  0x01
#define ADDR_STATUS 0x02
#define ADDR_CNTRL  0x03

#define AMP_CTRL_ENABLE   (1 << 0)
#define AMP_CTRL_MUTE     (1 << 1)

extern QueueHandle_t q_blink;
void task_amp_init(void);
void audio_horn_on(void);
void audio_horn_off(void);

typedef enum {
    AMP_GAIN_20DB = 0,
    AMP_GAIN_26DB,
    AMP_GAIN_32DB,
    AMP_GAIN_36DB
} amp_gain_t;

typedef enum {
    SG_5V = 0,
    SG_5_9V,
    SG_7V,
    SG_8_4V,
    SG_9_8V,
    SG_11_8V,
    SG_14V,
    SG_DISABLED
} amp_sg_level_t;

typedef enum {
    AMP_SW_400KHZ = 0,
    AMP_SW_500KHZ
} amp_sw_freq_t;

#endif