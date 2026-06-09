/*$T src/ssd1351.h GC 1.150 2023-03-18 10:52:22 */

//MIT License
//Copyright (c) 2022 Jaime Centeno
//Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
// to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifndef SSD1351_H
#define SSD1351_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>
#include "ece453_pins.h"
#include "../task_lcd.h"
#include "fonts.h"
#include "sprites.h"
#include "color_palette.h"

/*------- CONFIGURE THIS TO YOUR OWN HARDWARE AND HARDWARE ABSTRACTION LAYER -------*/

#include "ece453_pins.h"
#include "spi.h"

/* Wiring (SCB5 SPI master, controller board):
 *   MOSI  -> P5_0     D/C   -> P5_4
 *   SCLK  -> P5_2     RESET -> P6_5
 *   CS    -> P5_3
 *
 * The concrete pin IDs live in ece453_pins.h as LCD_PIN_*. The PORT macros
 * are kept only to satisfy the driver's two-arg SetPin/ClearPin API -- they
 * are ignored because cyhal_gpio takes a single pin identifier.
 */
// #define RESET_PORT  0
// #define RESET_PIN   LCD_PIN_RESET
// #define DC_PORT     0
// #define DC_PIN      LCD_PIN_DC
// #define CS_PORT     0
// #define CS_PIN      LCD_PIN_CS

#define OLED_128x128

/* SPI writes go over the shared mSPI handle brought up by spi_init() (spi.c).
 *
 * Using cyhal_spi_send (fully polled, no ISR dependency). The interrupt-driven
 * cyhal_spi_transfer path was observed to hang in the async busy-wait because
 * the SCB5 completion ISR was never invoked (txBufIdx stayed at 0). Polled
 * mode sidesteps that entirely. Buffer writes loop the same primitive -- at
 * 1 MHz the ~32 KB frame update still fits in ~250 ms.
 */

#define SSD1351_SendByte(data)                                              \
    do {                                                                    \
        uint8_t _ssd_b = (uint8_t)(data);                                   \
        cyhal_spi_transfer(&mSPI, &_ssd_b, 1, NULL, 0, 0xFF);               \
    } while (0)

#define SSD1351_SendBuffer(buffer, len)                                     \
    do {                                                                    \
        for (uint32_t _i = 0; _i < (uint32_t)(len); _i++) {                 \
            uint8_t _ssd_b = (uint8_t)((buffer)[_i]);                       \
            cyhal_spi_transfer(&mSPI, &_ssd_b, 1, NULL, 0, 0xFF);           \
        }                                                                   \
    } while (0)

/* GPIO: PORT arg is unused; cyhal_gpio_write keys on the pin identifier. */
#define SSD1351_SetPin(PORT, PIN)   cyhal_gpio_write((PIN), true)
#define SSD1351_ClearPin(PORT, PIN) cyhal_gpio_write((PIN), false)

#define SSD1351_DelayMs(x)  cyhal_system_delay_ms(x)

/*------------------- END OF HARDWARE CONFIGURATION ------------------------------- */

// Static definition for testing purposes on Ceedling
#ifdef TEST
#define STATIC
#else
#define STATIC	static
#endif // TEST
#ifdef OLED_128x128
#define DRAM_SIZE_8		32768
#define DRAM_SIZE_16	16384
#define COLUMNS			128
#define ROWS			128
#endif // OLED_128x128
typedef union DisplayRAM
{
	uint8_t		byte[DRAM_SIZE_8];
	uint16_t	halfw[DRAM_SIZE_16];
} DRAM;

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN		0x15
#define SSD1351_CMD_SETROW			0x75
#define SSD1351_CMD_WRITERAM		0x5C
#define SSD1351_CMD_READRAM			0x5D
#define SSD1351_CMD_SETREMAP		0xA0
#define SSD1351_CMD_STARTLINE		0xA1
#define SSD1351_CMD_DISPLAYOFFSET	0xA2
#define SSD1351_CMD_DISPLAYALLOFF	0xA4
#define SSD1351_CMD_DISPLAYALLON	0xA5
#define SSD1351_CMD_NORMALDISPLAY	0xA6
#define SSD1351_CMD_INVERTDISPLAY	0xA7
#define SSD1351_CMD_FUNCTIONSELECT	0xAB
#define SSD1351_CMD_DISPLAYOFF		0xAE
#define SSD1351_CMD_DISPLAYON		0xAF
#define SSD1351_CMD_PRECHARGE		0xB1
#define SSD1351_CMD_DISPLAYENHANCE	0xB2
#define SSD1351_CMD_CLOCKDIV		0xB3
#define SSD1351_CMD_SETVSL			0xB4
#define SSD1351_CMD_SETGPIO			0xB5
#define SSD1351_CMD_PRECHARGE2		0xB6
#define SSD1351_CMD_SETGRAY			0xB8
#define SSD1351_CMD_USELUT			0xB9
#define SSD1351_CMD_PRECHARGELEVEL	0xBB
#define SSD1351_CMD_VCOMH			0xBE
#define SSD1351_CMD_CONTRASTABC		0xC1
#define SSD1351_CMD_CONTRASTMASTER	0xC7
#define SSD1351_CMD_MUXRATIO		0xCA
#define SSD1351_CMD_COMMANDLOCK		0xFD
#define SSD1351_CMD_HORIZSCROLL		0x96
#define SSD1351_CMD_STOPSCROLL		0x9E
#define SSD1351_CMD_STARTSCROLL		0x9F

// Some color definitions
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_AQUA   0x07FF
#define COLOR_PURPLE 0xF81F
#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0xFFFF

#define SSD_PRINTF(...) SSD1351_printf(COLOR_WHITE, small_font, __VA_ARGS__)

extern struct cursor	SSD1351_cursor;

uint16_t				SSD1351_get_rgb(uint8_t r, uint8_t g, uint8_t b);

void					SSD1351_init(void);

void					SSD1351_stop(void);

void					SSD1351_turn_off(void);

void					SSD1351_fill(uint16_t color);

void					SSD1351_write_pixel(int16_t x, int16_t y, uint16_t color);

void					SSD1351_update(void);

void					SSD1351_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void					SSD1351_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void					SSD1351_draw_filled_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void					SSD1351_draw_circle(int16_t xc, int16_t yc, uint16_t r, uint16_t color);

void					SSD1351_draw_filled_circle(int16_t xc, int16_t yc, uint16_t r, uint16_t color);

void					SSD1351_printf(uint16_t color, const font_t *font, const char *format, ...);

void					SSD1351_set_cursor(uint8_t x, uint8_t y);

void					SSD1351_draw_sprite(int16_t x, int16_t y, sprite *sp);

void                    SSD1351_fill_screen(uint8_t red, uint8_t green, uint8_t blue);

#endif //SSD1351_H
