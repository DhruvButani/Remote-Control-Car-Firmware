#include "task_acc.h"
#include "task_console.h"
#include "task_sendCarPacket.h"   /* packet report queue */
#include "i2c.h"                  /* I2C_MASTER_FREQUENCY, cyhal_i2c types */
#include "ece453_pins.h"          /* MOD_2_PIN_I2C_* */
#include "lsm6dsm_reg.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

QueueHandle_t q_acc;

/* Private I2C bus for the LSM6DSM on MODULE_SITE_2.  Only task_acc
 * ever touches this bus, so no mutex is needed — and creating one
 * before vTaskStartScheduler() (as the init path required) led to the
 * shim wedging on xSemaphoreTake() and silently dropping every
 * transaction, which is why WHO_AM_I read back as 0.                 */
static cyhal_i2c_t acc_i2c_obj;

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_acc(void *param);

static int32_t acc_i2c_write(void *handle, uint8_t reg,
                             const uint8_t *buf, uint16_t len);
static int32_t acc_i2c_read (void *handle, uint8_t reg,
                             uint8_t *buf, uint16_t len);

static BaseType_t cli_handler_acc(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/******************************************************************************/
/* Driver Context                                                             */
/******************************************************************************/

/* stmdev driver context — wires the ST register-level driver to our shims. */
static stmdev_ctx_t lsm6dsm_ctx =
{
    .write_reg = acc_i2c_write,
    .read_reg  = acc_i2c_read,
    .mdelay    = NULL,
    .handle    = NULL,
};

/* CLI command definition */
static const CLI_Command_Definition_t xAcc =
{
    "acc",
    "\r\nacc\r\n Trigger a one-shot LSM6DSM accelerometer read\r\n",
    cli_handler_acc,
    0
};

/******************************************************************************/
/* Static Helper Functions                                                    */
/******************************************************************************/

/**
 * @brief stmdev write shim — single-byte register address followed by
 *        the data payload, all in one I2C write transaction (auto-
 *        increment is enabled in CTRL3_C so multi-byte writes walk
 *        forward through the register map).
 */
static int32_t acc_i2c_write(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len) {

    uint8_t tx[1 + 16];
    (void)handle;
    if (len > sizeof(tx) - 1U) return -1;
    tx[0] = reg;
    memcpy(&tx[1], buf, len);
    return (cyhal_i2c_master_write(&acc_i2c_obj, ACC_I2C_ADDR, tx, (uint16_t)(len + 1U), 100, true) == CY_RSLT_SUCCESS) ? 0 : -1;
}

/**
 * @brief stmdev read shim — write the register address with no stop,
 *        then repeated-start read.  Matches the LSM6DSM I2C protocol
 *        from the datasheet (§4.2 figure 5).
 */
static int32_t acc_i2c_read(void *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    cy_rslt_t result;
    (void)handle;
    result = cyhal_i2c_master_write(&acc_i2c_obj, ACC_I2C_ADDR, &reg, 1, 100, false);

    if (result != CY_RSLT_SUCCESS) return -1;
    return (cyhal_i2c_master_read(&acc_i2c_obj, ACC_I2C_ADDR, buf, len, 100, true) == CY_RSLT_SUCCESS) ? 0 : -1;
}

/******************************************************************************/
/* Static Task Definition                                                     */
/******************************************************************************/

/**
 * @brief Hardware management task for the LSM6DSM accelerometer.
 *
 * Blocks on q_acc waiting for a read request.  When one arrives:
 *   1. Poll XL data-ready until a fresh sample is available.
 *   2. Read raw X/Y/Z, convert to mg using the ±2 g sensitivity helper.
 *   3. Either print the magnitude (CLI request) or post a packed 6-bit
 *      code to q_carpacket_report (packet request).
 */
static void task_acc(void *param)
{
    acc_message_type_t message;
    int16_t  raw[3];
    uint8_t  drdy;

    (void)param;

    for (;;)
    {
        xQueueReceive(q_acc, &message, portMAX_DELAY);

        if (message != ACC_READ && message != ACC_READ_PACKET)
            continue;

        /* Poll XL data-ready (bounded so we never hang the task). */
        for (int i = 0; i < 50; i++)
        {
            drdy = 0;
            (void)lsm6dsm_xl_flag_data_ready_get(&lsm6dsm_ctx, &drdy);
            if (drdy) break;
            vTaskDelay(pdMS_TO_TICKS(2));
        }

        if (lsm6dsm_acceleration_raw_get(&lsm6dsm_ctx, raw) != 0)
        {
            task_print_info("ACC: raw read failed\r\n");
            continue;
        }

        /* Convert each axis to milli-g (configured for ±2 g full scale). */
        float ax_mg = lsm6dsm_from_fs2g_to_mg(raw[0]);
        float ay_mg = lsm6dsm_from_fs2g_to_mg(raw[1]);
        float az_mg = lsm6dsm_from_fs2g_to_mg(raw[2]);

        float mag_mg = sqrtf(ax_mg * ax_mg + ay_mg * ay_mg + az_mg * az_mg);

        /* Quantise magnitude into a 6-bit code. */
        uint32_t code = (uint32_t)(mag_mg / (float)ACC_PACKET_MG_PER_LSB);
        if (code > ACC_PACKET_MAX_CODE) code = ACC_PACKET_MAX_CODE;

        if (message == ACC_READ)
        {
            task_print_info(
                "ACC: |a|=%lu mg  [X=%ld Y=%ld Z=%ld mg]  code=0x%02lX\r\n",
                (unsigned long)mag_mg,
                (long)ax_mg, (long)ay_mg, (long)az_mg,
                (unsigned long)code);
        }
        else /* ACC_READ_PACKET */
        {
            packet_sensor_report_t report = {
                .sensor_id = PKT_SENSOR_ACC,
                .value     = code
            };
            (void)xQueueSendToBack(q_carpacket_report, &report, 0);
        }
    }
}

/**
 * @brief FreeRTOS CLI handler for the "acc" command.
 */
static BaseType_t cli_handler_acc(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    acc_message_type_t msg = ACC_READ;

    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    xQueueSendToBack(q_acc, &msg, portMAX_DELAY);

    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    return pdFALSE;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Initialise hardware and FreeRTOS objects for the LSM6DSM task.
 *
 * Must be called once from main() before vTaskStartScheduler().
 */
void task_acc_init(void)
{

    uint8_t   whoami = 0;

    /* Must create custom i2c master/obj -> rebinding with i2c_init(Modulesite)*/
    
    cyhal_i2c_cfg_t cfg = { CYHAL_I2C_MODE_MASTER, 0, I2C_MASTER_FREQUENCY };
    cyhal_i2c_init(&acc_i2c_obj, MOD_2_PIN_I2C_SDA, MOD_2_PIN_I2C_SCL, NULL);
    cyhal_i2c_configure(&acc_i2c_obj, &cfg);

    cyhal_system_delay_ms(50);   // bumped from 10 — LSM6DSM boot is ~35ms

    lsm6dsm_device_id_get(&lsm6dsm_ctx, &whoami);
    //task_print_info("ACC: WHO_AM_I   rslt=%ld  whoami=0x%02X (expect 0x%02X)\r\n", (long)result, whoami, LSM6DSM_ID);

    /* Software reset — poll the reset bit until it self-clears. */
    {
        uint8_t rst = 0;
        (void)lsm6dsm_reset_set(&lsm6dsm_ctx, PROPERTY_ENABLE);
        do {
            (void)lsm6dsm_reset_get(&lsm6dsm_ctx, &rst);
        } while (rst);
    }

    /* Block-data-update + register address auto-increment. */
    (void)lsm6dsm_block_data_update_set(&lsm6dsm_ctx, PROPERTY_ENABLE);
    (void)lsm6dsm_auto_increment_set  (&lsm6dsm_ctx, PROPERTY_ENABLE);

    /* Accelerometer: ±2 g full-scale, 104 Hz output data rate. */
    (void)lsm6dsm_xl_full_scale_set(&lsm6dsm_ctx, LSM6DSM_2g);
    (void)lsm6dsm_xl_data_rate_set (&lsm6dsm_ctx, LSM6DSM_XL_ODR_104Hz);

    cyhal_system_delay_ms(20);

    /* Queue depth of 1 — only one pending read request at a time. */
    q_acc = xQueueCreate(1, sizeof(acc_message_type_t));
    configASSERT(q_acc != NULL);

    FreeRTOS_CLIRegisterCommand(&xAcc);

    xTaskCreate(task_acc, "Task_ACC",
                configMINIMAL_STACK_SIZE * 2, NULL,
                configMAX_PRIORITIES - 6, NULL);
}
