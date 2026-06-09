#ifndef __TASK_LCD_H__
#define __TASK_LCD_H__

#include "main.h"
#include "task_bt_packet.h"

extern cyhal_spi_t mSPI;

/* Bring up SCB5 SPI + LCD GPIOs, initialize the SSD1351 panel, and create the
 * LCD FreeRTOS task. Call once from main() after cybsp_init(). */
void vLCDTask_init(void);


#endif
