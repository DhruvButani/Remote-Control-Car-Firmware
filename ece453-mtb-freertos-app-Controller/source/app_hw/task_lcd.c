/**
 * @file task_lcd.c
 * @brief Minimal bring-up for the SSD1351 OLED on SCB5 -- draws a single
 *        circle to verify the panel is wired correctly.
 */

#include "task_lcd.h"
#include "ece453_pins.h"
// #include "ssd1351DriverFiles/ssd1351.h"


cyhal_spi_t mSPI;

void vLCDTask_init(void)
{
    // cy_rslt_t rslt;

    // /* SPI master on SCB5. CS is driven as a GPIO by the driver, so pass NC. */
    // rslt = cyhal_spi_init(&mSPI,LCD_PIN_SPI_MOSI, NC, LCD_PIN_SPI_SCLK, NC, NULL, 8, CYHAL_SPI_MODE_00_MSB, false);
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    // cyhal_spi_set_frequency(&mSPI, 4000000);   // 1 MHz
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    // rslt = cyhal_gpio_init(LCD_PIN_CS,    CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    // rslt = cyhal_gpio_init(LCD_PIN_DC,    CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    // rslt = cyhal_gpio_init(LCD_PIN_RESET, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    // SSD1351_init();
    // SSD1351_DelayMs(100);
    // SSD1351_fill_screen(0x05, 0x3F, 0x1C); 


}
