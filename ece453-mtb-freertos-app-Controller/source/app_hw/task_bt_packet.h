#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

// A struct to keep all dashboard data in one place
typedef struct {
    uint8_t  speed;
    bool     horn_active;
    bool     auto_brake_active;
    bool     headlights_on;
} lcd_info_t;

// Function prototypes to get/set data safely
void update_vehicle_speed(uint8_t new_speed);
void update_horn_status(bool active);
lcd_info_t get_lcd_info(void);
void status_cli_init(void);

#endif