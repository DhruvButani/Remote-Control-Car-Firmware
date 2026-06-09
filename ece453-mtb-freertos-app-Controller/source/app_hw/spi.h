/*
 *  Created on: Jan 18, 2022
 *      Author: Joe Krachey
 *
 *  The shared `mSPI` handle is now defined in task_lcd.c (see note in spi.c).
 *  This header only re-exports the extern so the SSD1351 driver, which
 *  hardcodes `#include "spi.h"` and references `mSPI` via its SendByte
 *  macros, keeps compiling unchanged.
 */

#ifndef SPI_H__
#define SPI_H__

#include "main.h"
#include "ece453_pins.h"

/* Defined in task_lcd.c. */
// extern cyhal_spi_t mSPI;

#endif
