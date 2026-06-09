/*
 * spi.c
 *
 * The shared SPI handle `mSPI` and its bring-up used to live here. It was
 * moved to task_lcd.c, which now owns SCB5 directly via cyhal_spi_init()
 * (see "don't use spi.c for the LCD" requirement). This file is intentionally
 * left as a stub so the existing build target keeps linking; if/when another
 * SPI peripheral comes back, give it its own handle rather than reintroducing
 * a global here.
 */

#include "spi.h"
