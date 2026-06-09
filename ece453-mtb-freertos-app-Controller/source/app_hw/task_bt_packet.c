#include "task_bt_packet.h"

// The actual memory storage for our data
static lcd_info_t core_state = {
    .speed = 0,
    .horn_active = false,
    .auto_brake_active = false,
    .headlights_on = false,
};

void update_vehicle_speed(uint8_t new_speed)
{
    core_state.speed = new_speed;
}

void update_horn_status(bool active)
{
    core_state.horn_active = active;
}

void update_auto_brake_status(bool active)
{
    core_state.auto_brake_active = active;
}

void update_headlights_status(bool active)
{
    core_state.headlights_on = active;
}

lcd_info_t get_lcd_info(void)
{
    return core_state;
}

// Forward declaration of the command handler
static BaseType_t cli_cmd_status(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

// Command structure definition
static const CLI_Command_Definition_t status_command = {
    "status", // The command string the user types
    "\r\nstatus:\r\n Displays current controller event bitfield states\r\n",
    cli_cmd_status, // The function that processes the command
    0               // Number of parameters expected
};

/**
 * @brief Initializes and registers the Status CLI command.
 * Call this once in your main() or initialization task.
 */
void status_cli_init(void)
{
    FreeRTOS_CLIRegisterCommand(&status_command);
}

/**
 * @brief Actual logic for the 'status' command.
 * Formats the bitfields from controller_events.h into the CLI output buffer.
 */
static BaseType_t cli_cmd_status(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    /* Remove compiler warning about unused parameter */
    (void)pcCommandString;

    /* Create a header for the output */
    const char *header = "\r\n--- Controller Event Status ---\r\n";

    /* Format the bitfield values into the buffer.
     * Using %u for the 1-bit unsigned integers. */
    snprintf(pcWriteBuffer, xWriteBufferLen,
             "\r\n[EVENTS] H:%u P:%u R:%u\r\n"
             "ACC:%u BRK:%u GER:%s\r\n"
             "LEFT:%u RIGHT:%u REC:%u\r\n",
             Controller_Events.horn,
             Controller_Events.pair,
             Controller_Events.reset,
             Controller_Events.accelerate,
             Controller_Events.brake,
             Controller_Events.gear_sel ? "REV" : "FWD",
             Controller_Events.turn_left,
             Controller_Events.turn_right,
             Controller_Events.record);

    /* Return pdFALSE to indicate the entire response has been
     * placed in the buffer and no further calls are needed. */
    return pdFALSE;
}

/**TODO:
 * SEND Controller_Events info to the car via bluetooth, and recieve speed updates from the car to update the LCD.
 */
/** Create task that prints off status of controller events.h. */