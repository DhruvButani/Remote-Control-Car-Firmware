#include "task_tof.h"
#include "ece453_pins.h"

/* VL53LX driver – relative paths from source/app_hw/ to source/vl53l3cx/ */
#include "../vl53l3cx/core/inc/vl53lx_api.h"
#include "../vl53l3cx/platform/inc/vl53lx_platform.h"
#include "../vl53l3cx/platform/inc/vl53lx_platform_init.h"
#include "task_console.h"
#include "task_sendCarPacket.h"

/******************************************************************************/
/* Sensor configuration                                                       */
/******************************************************************************/
#define TOF_PIN_XSHUT           MOD_1_PIN_IO_0   /* P9_4 – active-high enable */
#define TOF_I2C_ADDR            (0x29U)           /* 7-bit I2C address         */
#define TOF_I2C_SPEED_KHZ       (400U)            /* 400 kHz fast-mode         */
#define TOF_TIMING_BUDGET_US    (50000U)          /* 50 ms timing budget       */
#define TOF_POLL_DELAY_MS       (10U)             /* polling interval (ms)     */

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_tof(void *param);

static BaseType_t cli_handler_tof(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

/* Queue used to send commands to the ToF task */
QueueHandle_t q_tof;

/* VL53LX device handle – holds I2C address, speed, and driver state */
static VL53LX_Dev_t tof_dev;

/* CLI command definition */
static const CLI_Command_Definition_t xTof =
{
    "tof",                                                    /* command text  */
    "\r\ntof\r\n Trigger a one-shot VL53LX distance read\r\n", /* help text   */
    cli_handler_tof,                                          /* handler       */
    0                                                         /* no parameters */
};

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Hardware management task for the VL53LX sensor.
 *
 * Blocks on q_tof waiting for a TOF_READ request.  When one arrives:
 *   1. Poll VL53LX_GetMeasurementDataReady until the sensor signals ready,
 *      yielding to the RTOS for TOF_POLL_DELAY_MS between each check.
 *   2. Call VL53LX_GetMultiRangingData and print the primary target distance.
 *   3. Call VL53LX_ClearInterruptAndStartMeasurement to re-arm the sensor.
 */
static void task_tof(void *param)
{
    tof_message_type_t message;
    VL53LX_Error status;
    VL53LX_MultiRangingData_t data;
    uint8_t data_ready;

    (void)param;

    for (;;)
    {
        /* Block until a read request arrives on the queue */
        xQueueReceive(q_tof, &message, portMAX_DELAY);

        if(message == TOF_READ || message == TOF_READ_PACKET) {

            VL53LX_MultiRangingData_t data;
            uint8_t ready = 0;

            for (int i = 0; i < 3; i++) {

                do {
                    status = VL53LX_GetMeasurementDataReady(&tof_dev, &ready);

                    if(status != VL53LX_ERROR_NONE) {
                        task_print_info("Failed to get measurement data ready: %d\r\n", status);
                        break;
                    }

                    if(!ready) {
                        vTaskDelay(pdMS_TO_TICKS(TOF_POLL_DELAY_MS));
                    }
                } while (!ready);

                status = VL53LX_GetMultiRangingData(&tof_dev, &data);
                if (status != VL53LX_ERROR_NONE) {
                    task_print_info("Failed to get ranging measurement data: %d\r\n", status);
                    break;
                }

                VL53LX_ClearInterruptAndStartMeasurement(&tof_dev);
            }

            int32_t distance_mm = data.RangeData[0].RangeMilliMeter;

            if (message == TOF_READ) {
                task_print_info("Distance: %d mm\r\n", distance_mm);
            }
            else /* TOF_READ_PACKET */ {
                packet_sensor_report_t report = {
                    .sensor_id = PKT_SENSOR_TOF,
                    .value     = (distance_mm < 0) ? 0xFFFFFFFFu
                                                   : (uint32_t)distance_mm
                };
                (void)xQueueSendToBack(q_carpacket_report, &report, 0);
            }
        }

        cyhal_system_delay_ms(10);

    }
}

/**
 * @brief FreeRTOS CLI handler for the "tof" command.
 *
 * Sends a TOF_READ token to q_tof.  The task prints the result
 * asynchronously, so the write buffer is left empty and pdFALSE is
 * returned immediately to signal command completion to the CLI engine.
 */
static BaseType_t cli_handler_tof(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    tof_message_type_t tof_type = TOF_READ;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    /* Queue the read request to the hardware-management task */
    xQueueSendToBack(q_tof, &tof_type, portMAX_DELAY);

    /* No synchronous output – task will print the result when ready */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    return pdFALSE;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Initialise all hardware resources and FreeRTOS objects for the ToF task.
 *
 * Must be called once from main() before vTaskStartScheduler().
 */


void task_tof_init(void)
{
    VL53LX_Error status;
    
    // tof_dev.pin_scl = TOF_PIN_SCL;
    // tof_dev.pin_sda = TOF_PIN_SDA;
    // tof_dev.pin_xshut = TOF_PIN_XSHUT;

    status = VL53LX_platform_init(&tof_dev, TOF_I2C_ADDR, 0, TOF_I2C_SPEED_KHZ);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: 0,


    status = VL53LX_WaitDeviceBooted(&tof_dev);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: -3, 


    status = VL53LX_DataInit(&tof_dev);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: -3, 

    status = VL53LX_SetDistanceMode(&tof_dev, VL53LX_DISTANCEMODE_LONG);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: -15,


    status = VL53LX_SetMeasurementTimingBudgetMicroSeconds(
        &tof_dev, TOF_TIMING_BUDGET_US);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: 0,


    status = VL53LX_StartMeasurement(&tof_dev);
    CY_ASSERT(VL53LX_ERROR_NONE == status); //Status: -13,

    /* Queue depth of 1 – only one pending read request at a time */
    q_tof = xQueueCreate(1, sizeof(tof_message_type_t));

    /* Register the "tof" CLI command */
    FreeRTOS_CLIRegisterCommand(&xTof);

    /* Create the hardware-management task */
    xTaskCreate(
        task_tof,
        "Task_ToF",
        configMINIMAL_STACK_SIZE * 4, /* extra stack for VL53LX data structs */
        NULL,
        configMAX_PRIORITIES - 6,
        NULL);
}
