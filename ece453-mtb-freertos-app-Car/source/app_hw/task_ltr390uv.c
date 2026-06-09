#include "task_ltr390uv.h"
#include "task_console.h"
#include "task_sendCarPacket.h"   /* packet report queue */
#include "i2c.h"   /* provides i2c_master_obj and Semaphore_I2C */

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

QueueHandle_t q_ltr390uv;

/* NOTE: No private cyhal_i2c_t here — we use the shared i2c_master_obj
 * from i2c.c to avoid double-initialising the same SCB hardware block.    */

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_ltr390uv(void *param);

static BaseType_t cli_handler_ltr390uv(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/******************************************************************************/
/* CLI command definition                                                     */
/******************************************************************************/
static const CLI_Command_Definition_t xLtr390uv =
{
    "ltr390uv",
    "\r\nltr390uv\r\n Trigger a one-shot LTR-390UV-01 light reading\r\n",
    cli_handler_ltr390uv,
    0
};

/******************************************************************************/
/* Static Helper Functions                                                    */
/******************************************************************************/

/**
 * @brief Write one byte to a sensor register over the shared I2C bus.
 *
 * Takes Semaphore_I2C before the transaction and gives it back after,
 * matching the mutual-exclusion pattern established in i2c.c.
 */
static cy_rslt_t ltr390uv_write_reg(uint8_t reg, uint8_t value)
{
    cy_rslt_t result;
    uint8_t   buf[2] = { reg, value };

    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    result = cyhal_i2c_master_write(
        &i2c_master_obj,
        LTR390UV_I2C_ADDR,
        buf, sizeof(buf),
        100,   /* timeout ms — never 0, avoids infinite hang on NAK */
        true);

    xSemaphoreGive(Semaphore_I2C);
    return result;
}

/**
 * @brief Read one byte from a sensor register over the shared I2C bus.
 *
 * Uses a repeated-start (write reg addr with no stop, then read) as
 * required by the LTR-390UV-01 Block Read protocol (datasheet §5.1 IV).
 * The semaphore is held across both transfers so no other task can
 * interleave on the bus between the write and the read.
 */
static cy_rslt_t ltr390uv_read_reg(uint8_t reg, uint8_t *value)
{
    cy_rslt_t result;

    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    /* Write register address — no stop, repeated start follows */
    result = cyhal_i2c_master_write(
        &i2c_master_obj,
        LTR390UV_I2C_ADDR,
        &reg, 1,
        100,
        false);   /* false = no stop condition */

    if (result == CY_RSLT_SUCCESS)
    {
        result = cyhal_i2c_master_read(
            &i2c_master_obj,
            LTR390UV_I2C_ADDR,
            value, 1,
            100,
            true);
    }

    xSemaphoreGive(Semaphore_I2C);
    return result;
}

/******************************************************************************/
/* Static Task Definition                                                     */
/******************************************************************************/

/**
 * @brief Hardware management task for the LTR-390UV-01 sensor.
 *
 * Blocks on q_ltr390uv waiting for an LTR390UV_READ request.  When one
 * arrives:
 *   1. Poll MAIN_STATUS bit 3 until new data is ready.
 *   2. Read the three ALS data registers and assemble the 20-bit count.
 *   3. Print "Light: ON" / "Light: OFF" based on the threshold.
 */
static void task_ltr390uv(void *param)
{
    ltr390uv_message_type_t message;
    cy_rslt_t               result;
    uint8_t                 status_reg;
    uint8_t                 d0, d1, d2;
    uint32_t                als_count;

    (void)param;

    for (;;)
    {
        xQueueReceive(q_ltr390uv, &message, portMAX_DELAY);

        if (message == LTR390UV_READ || message == LTR390UV_READ_PACKET)
        {
            /* ── Poll until new ALS data is available ────────────── */
            do
            {
                result = ltr390uv_read_reg(LTR390UV_REG_MAIN_STATUS,
                                           &status_reg);
                if (result != CY_RSLT_SUCCESS)
                {
                    task_print_info(
                        "LTR390UV: failed to read status (0x%lx)\r\n",
                        (unsigned long)result);
                    break;
                }

                if (!(status_reg & LTR390UV_STATUS_DATA_READY))
                    vTaskDelay(pdMS_TO_TICKS(LTR390UV_POLL_DELAY_MS));

            } while (!(status_reg & LTR390UV_STATUS_DATA_READY));

            if (result != CY_RSLT_SUCCESS) continue;

            /* ── Read three ALS data registers (LSB first) ────────── */
            ltr390uv_read_reg(LTR390UV_REG_ALS_DATA_0, &d0);
            ltr390uv_read_reg(LTR390UV_REG_ALS_DATA_1, &d1);
            ltr390uv_read_reg(LTR390UV_REG_ALS_DATA_2, &d2);

            als_count = ((uint32_t)(d2 & 0x0F) << 16) |
                        ((uint32_t) d1          <<  8) |
                         (uint32_t) d0;

            if (message == LTR390UV_READ)
            {
                if (als_count > LTR390UV_LUX_THRESHOLD_COUNTS)
                    task_print_info("Light: ON  (ALS count = %lu)\r\n",
                                    (unsigned long)als_count);
                else
                    task_print_info("Light: OFF (ALS count = %lu)\r\n",
                                    (unsigned long)als_count);
            }
            else /* LTR390UV_READ_PACKET */
            {
                packet_sensor_report_t report = {
                    .sensor_id = PKT_SENSOR_LTR390UV,
                    .value     = als_count
                };
                (void)xQueueSendToBack(q_carpacket_report, &report, 0);
            }
        }
    }
}

/**
 * @brief FreeRTOS CLI handler for the "ltr390uv" command.
 */
static BaseType_t cli_handler_ltr390uv(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    ltr390uv_message_type_t msg = LTR390UV_READ;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    xQueueSendToBack(q_ltr390uv, &msg, portMAX_DELAY);

    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    return pdFALSE;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Initialise all hardware resources and FreeRTOS objects for the
 *        LTR-390UV-01 task.
 *
 * Assumes i2c_init(MODULE_SITE_0) has already been called from main()
 * before this function is invoked.
 *
 * Must be called once from main() before vTaskStartScheduler().
 */
void task_ltr390uv_init(void) {
    
    cy_rslt_t result;
    uint8_t buf[2];

    result = i2c_init(MODULE_SITE_0);
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    cyhal_system_delay_ms(10);

    /* Call cyhal directly here — scheduler not running yet,
     * semaphore cannot be used before vTaskStartScheduler()  */
    buf[0] = LTR390UV_REG_MEAS_RATE; buf[1] = LTR390UV_MEAS_REG_VAL;
    result = cyhal_i2c_master_write(&i2c_master_obj, LTR390UV_I2C_ADDR, buf, 2, 100, true);
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    
    buf[0] = LTR390UV_REG_GAIN; buf[1] = LTR390UV_GAIN_REG_VAL;
    result = cyhal_i2c_master_write(&i2c_master_obj, LTR390UV_I2C_ADDR, buf, 2, 100, true);
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    buf[0] = LTR390UV_REG_MAIN_CTRL; buf[1] = LTR390UV_CTRL_ALS_ACTIVE;
    result = cyhal_i2c_master_write(&i2c_master_obj, LTR390UV_I2C_ADDR, buf, 2, 100, true);
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* From here on the task runs — semaphore is correct in ltr390uv_write_reg/read_reg */
    q_ltr390uv = xQueueCreate(1, sizeof(ltr390uv_message_type_t));
    FreeRTOS_CLIRegisterCommand(&xLtr390uv);
    xTaskCreate(task_ltr390uv, "Task_LTR390UV", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 6, NULL);
}