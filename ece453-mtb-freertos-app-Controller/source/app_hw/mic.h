// #ifndef __TASK_MIC_H__
// #define __TASK_MIC_H__

// #include "main.h"
// #include "task_console.h"
// #include "i2c.h"
// #include <stdio.h>
// #include <stdint.h>
// #include "cy_result.h"
// #include <math.h>
// #include "cyhal_dac.h"

// // 8000 Hz * 5 seconds = 40,000 samples
// #define SAMPLING_RATE       8000
// #define RECORD_SECONDS      5
// #define BUFFER_SIZE         (SAMPLING_RATE * RECORD_SECONDS)
// #define DAC_MAX ((1 << 16) - 1)
// #define MOD_0_LOAD_SWITCH_1 MOD_0_PIN_IO_0 // P5_4

// typedef enum {
//     STATE_IDLE,
//     STATE_RECORDING,
//     STATE_PLAYBACK
// } audio_state_t;

// void audio_init(void);
// void audio_task(void); // Call this in your main loop

// #endif

