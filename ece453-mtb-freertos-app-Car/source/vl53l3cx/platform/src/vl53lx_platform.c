
// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved

 This file is part of VL53LX and is dual licensed,
 either GPL-2.0+
 or 'BSD 3-clause "New" or "Revised" License' , at your option.
 ******************************************************************************
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cyhal.h"
#include "cybsp.h"

/* FreeRTOS – needed for GetTickCount */
#include "FreeRTOS.h"
#include "task.h"

#include "vl53lx_platform.h"
#include <vl53lx_platform_log.h>

/******************************************************************************/
/* Platform hardware pin definitions (Module 1 connector)                     */
/******************************************************************************/
#define PIN_I2C_SDA    P9_1   /* I2C data  (MOD_1_PIN_I2C_SDA) */
#define PIN_I2C_SCL    P9_0    /* I2C clock (MOD_1_PIN_I2C_SCL) */
#define TOF_PLATFORM_PIN_XSHUT  P9_4    /* Shutdown  (MOD_1_PIN_IO_0)    */

/* Timeout passed to the HAL for each I2C transaction (ms) */
#define I2C_TIMEOUT_MS  50U

/******************************************************************************/
/* Module-level HAL objects                                                   */
/******************************************************************************/
// static cyhal_i2c_t *i2c;
static bool xshut_initialized = false;
static cyhal_i2c_t i2c_obj;  /* persistent I2C handle – must outlive CommsInitialise */


#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53LX_TRACE_MODULE_PLATFORM, \
	level, VL53LX_TRACE_FUNCTION_NONE, ##__VA_ARGS__)

#define trace_i2c(...) \
	_LOG_TRACE_PRINT(VL53LX_TRACE_MODULE_NONE, \
	VL53LX_TRACE_LEVEL_NONE, VL53LX_TRACE_FUNCTION_I2C, ##__VA_ARGS__)


VL53LX_Error VL53LX_CommsInitialise(
	VL53LX_Dev_t *pdev,
	uint8_t       comms_type,
	uint16_t      comms_speed_khz)
{
	VL53LX_Error    status = VL53LX_ERROR_NONE;
	cy_rslt_t       rslt;

	cyhal_i2c_cfg_t i2c_monarch_config = {
			CYHAL_I2C_MODE_MASTER,
			0, // address is not used for monarch mode
			(uint32_t)comms_speed_khz*1000U //I2C_MONARCH_FREQUENCY
		};

	(void)comms_type; /* I2C only on this platform */

	/* Initialise the I2C master on SCL=P9_0, SDA=P9_1 using the static handle */
	rslt = cyhal_i2c_init(&i2c_obj, P9_1, P9_0, NULL);
		
	if (rslt != CY_RSLT_SUCCESS)
		return VL53LX_ERROR_CONTROL_INTERFACE;

	/* Configure as master at the requested clock speed */
	rslt = cyhal_i2c_configure(&i2c_obj, &i2c_monarch_config);

	if (rslt != CY_RSLT_SUCCESS)
		status = VL53LX_ERROR_CONTROL_INTERFACE;

	/* Store the pointer so WriteMulti/ReadMulti can reach the handle */
	pdev->i2c = &i2c_obj;

	return status;
}


VL53LX_Error VL53LX_CommsClose(
	VL53LX_Dev_t *pdev)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}



VL53LX_Error VL53LX_WriteMulti(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata,
	uint32_t      count)
{
	VL53LX_Error  status = VL53LX_ERROR_NONE;
	cy_rslt_t     rslt;

	/*
	 * I2C write format: [reg_hi, reg_lo, data[0] ... data[count-1]]
	 * All bytes must go out in a single transaction, so prepend the
	 * 16-bit register index into a heap-allocated staging buffer.
	 */
	uint8_t buffer[count + 2];

	/* The first two bytes of the buffer are the register index */

	buffer[0] = (uint8_t)(index >> 8); // MS byte of the index
	buffer[1] = (uint8_t)(index & 0xFF);

	/* The remaining bytes of the buffer are the data to be written */
	memcpy(&buffer[2], pdata, count);
	
	/* Write the buffer to the device */
	rslt = cyhal_i2c_master_write(
	pdev->i2c,
	pdev->i2c_slave_address,
	buffer,
	count + 2,
	1000, // Timeout in ms
	true // Send a stop condition after the write
	);

	if (rslt != CY_RSLT_SUCCESS)
		status = VL53LX_ERROR_CONTROL_INTERFACE;

	return status;
}


VL53LX_Error VL53LX_ReadMulti(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata,
	uint32_t      count)
{
	VL53LX_Error status = VL53LX_ERROR_NONE;
	cy_rslt_t    rslt;
	
	uint8_t      index_buffer[2];
	index_buffer[0] = (uint8_t)(index >> 8);
	index_buffer[1] = (uint8_t)(index & 0xFF);

	/*
	 * Phase 1 – write register address with NO stop condition so the
	 *            bus stays claimed and the next transfer issues a
	 *            repeated START.
	 */
	rslt = cyhal_i2c_master_write(
		pdev->i2c, 
		pdev->i2c_slave_address, 
		index_buffer, 
		2, 
		1000 , // Timeout in ms false // No stop condition, we will read immediately after )
		false);

	if(rslt != CY_RSLT_SUCCESS)
		return VL53LX_ERROR_UNDEFINED;

	/* Phase 2 – read the requested bytes, then release the bus */
	
	rslt = cyhal_i2c_master_read(
		pdev->i2c, 
		pdev->i2c_slave_address,
		pdata, 
		count, 
		1000, // Timeout in ms
		true // Send a stop condition after the read
	);

	if (rslt != CY_RSLT_SUCCESS)
		status = VL53LX_ERROR_CONTROL_INTERFACE;

	return status;
}


VL53LX_Error VL53LX_WrByte(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint8_t       VL53LX_p_003)
{
	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint8_t  buffer[1];


	buffer[0] = (uint8_t)(VL53LX_p_003);

	status = VL53LX_WriteMulti(pdev, index, buffer, 1);

	return status;
}


VL53LX_Error VL53LX_WrWord(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint16_t      VL53LX_p_003)
{
	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint8_t  buffer[2];


	buffer[0] = (uint8_t)(VL53LX_p_003 >> 8);
	buffer[1] = (uint8_t)(VL53LX_p_003 &  0x00FF);

	status = VL53LX_WriteMulti(pdev, index, buffer, VL53LX_BYTES_PER_WORD);

	return status;
}


VL53LX_Error VL53LX_WrDWord(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint32_t      VL53LX_p_003)
{
	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint8_t  buffer[4];


	buffer[0] = (uint8_t) (VL53LX_p_003 >> 24);
	buffer[1] = (uint8_t)((VL53LX_p_003 &  0x00FF0000) >> 16);
	buffer[2] = (uint8_t)((VL53LX_p_003 &  0x0000FF00) >> 8);
	buffer[3] = (uint8_t) (VL53LX_p_003 &  0x000000FF);

	status = VL53LX_WriteMulti(pdev, index, buffer, VL53LX_BYTES_PER_DWORD);

	return status;
}


VL53LX_Error VL53LX_RdByte(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata)
{
	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint8_t  buffer[1];

	status = VL53LX_ReadMulti(pdev, index, buffer, 1);

	*pdata = buffer[0];

	return status;
}


VL53LX_Error VL53LX_RdWord(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint16_t     *pdata)
{
	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint8_t  buffer[2];

	status = VL53LX_ReadMulti(
					pdev,
					index,
					buffer,
					VL53LX_BYTES_PER_WORD);

	*pdata = (uint16_t)(((uint16_t)(buffer[0])<<8) + (uint16_t)buffer[1]);

	return status;
}


VL53LX_Error VL53LX_RdDWord(
	VL53LX_Dev_t *pdev,
	uint16_t      index,
	uint32_t     *pdata)
{
	VL53LX_Error status = VL53LX_ERROR_NONE;
	uint8_t  buffer[4];

	status = VL53LX_ReadMulti(
					pdev,
					index,
					buffer,
					VL53LX_BYTES_PER_DWORD);

	*pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

	return status;
}



VL53LX_Error VL53LX_WaitUs(
	VL53LX_Dev_t *pdev,
	int32_t       wait_us)
{
	(void)pdev;
	cyhal_system_delay_us((uint32_t)wait_us);
	return VL53LX_ERROR_NONE;
}


VL53LX_Error VL53LX_WaitMs(
	VL53LX_Dev_t *pdev,
	int32_t       wait_ms)
{
	(void)pdev;
	cyhal_system_delay_ms((uint32_t)wait_ms);
	return VL53LX_ERROR_NONE;
}



VL53LX_Error VL53LX_GetTimerFrequency(int32_t *ptimer_freq_hz)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}


VL53LX_Error VL53LX_GetTimerValue(int32_t *ptimer_count)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}




VL53LX_Error VL53LX_GpioSetMode(uint8_t pin, uint8_t mode)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}


VL53LX_Error  VL53LX_GpioSetValue(uint8_t pin, uint8_t value)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;

}


VL53LX_Error  VL53LX_GpioGetValue(uint8_t pin, uint8_t *pvalue)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}



VL53LX_Error  VL53LX_GpioXshutdown(uint8_t value)
{
	cy_rslt_t rslt;

	if (!xshut_initialized)
	{
		/* First call: configure the pin, use the requested level as the
		 * initial drive value so we don't glitch the line. */
		rslt = cyhal_gpio_init(
			TOF_PLATFORM_PIN_XSHUT,
			CYHAL_GPIO_DIR_OUTPUT,
			CYHAL_GPIO_DRIVE_STRONG,
			(bool)value);
		CY_ASSERT(CY_RSLT_SUCCESS == rslt);
		(void)rslt;
		xshut_initialized = true;
	}
	else
	{
		cyhal_gpio_write(TOF_PLATFORM_PIN_XSHUT, (bool)value);
	}

	return VL53LX_ERROR_NONE;
}


VL53LX_Error  VL53LX_GpioCommsSelect(uint8_t value)
{
	/* No comms-select pin on this hardware; I2C is the only interface. */
	(void)value;
	return VL53LX_ERROR_NONE;
}


VL53LX_Error  VL53LX_GpioPowerEnable(uint8_t value)
{
	/* No separate power-enable pin on this hardware; sensor is always
	 * powered through the module connector. */
	(void)value;
	return VL53LX_ERROR_NONE;
}


VL53LX_Error  VL53LX_GpioInterruptEnable(void (*function)(void), uint8_t edge_type)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}


VL53LX_Error  VL53LX_GpioInterruptDisable(void)
{
	VL53LX_Error status = 255;
	/* To be filled by customer according to the platform request. Return 0 if OK */
	return status;
}


VL53LX_Error VL53LX_GetTickCount(
		VL53LX_Dev_t *pdev,
		uint32_t *ptick_count_ms)
{
	VL53LX_Error status = VL53LX_ERROR_NONE;
	(void)pdev;

	/* Convert FreeRTOS ticks → milliseconds.
	 * Before vTaskStartScheduler() this returns 0, which is acceptable:
	 * WaitValueMaskEx will not advance its timeout counter but will still
	 * exit once the device acknowledges (found == 1). */
	*ptick_count_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

	trace_print(
		VL53LX_TRACE_LEVEL_DEBUG,
		"VL53LX_GetTickCount() = %5u ms;\n",
		*ptick_count_ms);

	return status;
}


VL53LX_Error VL53LX_WaitValueMaskEx(
	VL53LX_Dev_t *pdev,
	uint32_t      timeout_ms,
	uint16_t      index,
	uint8_t       value,
	uint8_t       mask,
	uint32_t      poll_delay_ms)
{


	VL53LX_Error status         = VL53LX_ERROR_NONE;
	uint32_t     start_time_ms   = 0;
	uint32_t     current_time_ms = 0;
	uint8_t      byte_value      = 0;
	uint8_t      found           = 0;
#ifdef VL53LX_LOG_ENABLE
	uint32_t     trace_functions = 0;
#endif



#ifdef VL53LX_LOG_ENABLE

	trace_i2c("WaitValueMaskEx(%5d, 0x%04X, 0x%02X, 0x%02X, %5d);\n",
		timeout_ms, index, value, mask, poll_delay_ms);
#endif



	VL53LX_GetTickCount(pdev, &start_time_ms);
	pdev->new_data_ready_poll_duration_ms = 0;



#ifdef VL53LX_LOG_ENABLE
	trace_functions = _LOG_GET_TRACE_FUNCTIONS();
#endif
	_LOG_SET_TRACE_FUNCTIONS(VL53LX_TRACE_FUNCTION_NONE);



	while ((status == VL53LX_ERROR_NONE) &&
		   (pdev->new_data_ready_poll_duration_ms < timeout_ms) &&
		   (found == 0))
	{
		status = VL53LX_WaitMs(
					pdev,
					poll_delay_ms);
		status = VL53LX_RdByte(
						pdev,
						index,
						&byte_value);

		if ((byte_value & mask) == value)
		{
			found = 1;
		}




		VL53LX_GetTickCount(pdev, &current_time_ms);
		pdev->new_data_ready_poll_duration_ms = current_time_ms - start_time_ms;
	}

#ifdef VL53LX_LOG_ENABLE
	_LOG_SET_TRACE_FUNCTIONS(trace_functions);
#endif
	if (found == 0 && status == VL53LX_ERROR_NONE)
		status = VL53LX_ERROR_TIME_OUT;

	return status;
}


