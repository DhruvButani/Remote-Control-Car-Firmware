# RC Car — Controller Firmware (`ece453-mtb-freertos-app-Controller`)

Firmware for the **handheld remote** half of the ECE 453 BLE remote-controlled
car. It reads the joystick, buttons, and microphone, sends drive commands to the
car, and displays the car's live status on an SSD1351 OLED.

In BLE terms the controller is the **GATT client / central**. It scans for and
connects to the car, writes drive commands to the car's `CarDataIn`
characteristic, and subscribes to `CarDataOut` notifications to receive status.
It also acts as a client for a **battery** service.

> This project is part of a two-board system. See the
> [top-level README](../README.md) for the overall architecture and the link
> protocol, and the [car README](../ece453-mtb-freertos-app-Car) for the other
> half.

## Platform

- **MCU / board:** Infineon PSoC&trade; 6 (CAT1A), board target
  `APP_ECE453_FREERTOS`
- **RTOS:** FreeRTOS
- **BLE stack:** AIROC&trade; / WICED BLE
  (`COMPONENTS = FREERTOS HARDFP WICED_BLE bat_client_lib`)
- **Toolchain:** `GCC_ARM`, `CONFIG = Debug` (defaults in the `Makefile`)

## How it boots

`main.c` performs a fixed startup sequence:

1. `cybsp_init()` — initialize the board and peripherals.
2. `task_console_init()` — bring up the UART console so `task_print*` works
   before BLE starts.
3. `cybt_platform_config_init()` — configure the BT platform.
4. `app_kvstore_bd_config()` — set up the flash block device used by kv-store
   for bonding storage.
5. `wiced_bt_stack_init(battery_client_management_callback, ...)` — start the
   BLE stack as a client.
6. `xTaskCreate(button_task, ...)` — start the button task that drives the BLE
   connect/pair flow, then create the default BT heap.
7. `status_cli_init()` — register console commands.
8. `vTaskStartScheduler()` — hand control to FreeRTOS.

Note: most hardware is **not** initialized in `main()`. Per the comments in the
source, peripheral init for the client happens inside `battery_client.c` once
the connection is established.

## Input model — `Controller_Events`

User input is collected into a single packed bitfield, `Controller_Events`
(see `source/controller_events.h`). Buttons are polled on a timer and the
joystick updates its position on change:

| Field | Source | Used for |
| --- | --- | --- |
| `horn` | SW1 falling edge | sound the horn |
| `pair` | SW2 falling edge | start BLE pair/connect |
| `reset` | SW3 falling edge | reset |
| `accelerate` | SW4 falling edge | throttle (sent to car) |
| `brake` | SW5 falling edge | brake |
| `turn_left` | joystick left | steering (sent to car) |
| `turn_right` | joystick right | steering (sent to car) |
| `gear_sel` | gear toggle | 0 = forward, 1 = reverse (sent to car) |
| `record` | joystick record button | record |

## Source layout

```
ece453-mtb-freertos-app-Controller/
├── main.c / main.h                ← startup, includes, BLE client init
├── Makefile                       ← TARGET / TOOLCHAIN / COMPONENTS
├── source/
│   ├── controller_events.h        ← Controller_Events bitfield definition
│   ├── FreeRTOS_CLI.[ch]          ← command-line interpreter for the console
│   ├── app_hw/                    ← input + display + task layer (see below)
│   └── app_bt/                    ← BLE client (CarData + battery), bonding
└── bsps/ + libs/ + deps/          ← ModusToolbox BSP, libraries, dependencies
```

### `source/app_hw/` — input, display, and tasks

| File(s) | Responsibility |
| --- | --- |
| `joystick.[ch]` | Read the analog joystick; updates `turn_left`/`turn_right`/`record` |
| `buttons.[ch]` | Debounce the push buttons; updates the SW1–SW5 event bits |
| `mic.[ch]` | Microphone input |
| `timer.[ch]` | Periodic timer used to poll buttons and pace BLE sends |
| `task_bt_send.[ch]` | ~200 ms timer that packs `accelerate`/`gear_sel` into a CarDataIn write |
| `task_bt_packet.[ch]` | BLE packet assembly/handling |
| `task_lcd.[ch]` | Render car status (speed, light, autobrake) to the OLED |
| `ssd1351DriverFiles/` | SSD1351 OLED driver: `ssd1351`, `fonts`, `sprites`, `color_palette` |
| `task_io_expander.[ch]` | I/O expander control |
| `task_blink.[ch]` | Heartbeat / status LED |
| `task_console.[ch]` | UART console + CLI plumbing |
| `i2c.[ch]`, `spi.[ch]` | I2C / SPI bus helpers (OLED is on SPI) |
| `ece453_pins.h` | Board pin assignments |

### `source/app_bt/` — Bluetooth layer

| File(s) | Responsibility |
| --- | --- |
| `car_data_client.[ch]` | Subscribe to `CarDataOut`, parse status, write `CarDataIn` |
| `battery_client.[ch]` | Battery service client (and connection/peripheral init) |
| `app_bt_bonding.[ch]` (under `bonding/`) | Persistent bonding (pairing keys in flash) |
| `app_serial_flash.c`, `app_internal_aux_flash.c` | Flash access for bonding |
| `app_bt_utils.[ch]`, `wiced_bt_gatt_util.h`, `heap_usage.c` | GATT/util helpers |

## BLE protocol (this side)

### Outbound — `CarDataIn` (controller writes)

`task_bt_send` runs on a ~200 ms timer. It packs the current
`Controller_Events` accelerate/gear_sel state into one byte and writes it to the
car's `CarDataIn` value handle (`car_data_send_control`):

| Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Field | gear_sel | turn_right | turn_left | brake | accelerate | reset | pair | horn |

(`record` is excluded from the packed byte.)

### Inbound — `CarDataOut` (controller receives notifications)

After connecting, the client subscribes to `CarDataOut` by writing `0x0001` to
its CCCD (`car_data_subscribe_notifications`). Each notification is one byte,
parsed into `car_data_out_t`:

| Bits | 7 | 6 | 5:0 |
| --- | --- | --- | --- |
| Field | autobrake | light | speed (0–63) |

The parsed values drive the LCD: speed (0–63), the light indicator, and the
autobrake warning.

## Building and flashing

```bash
make getlibs     # fetch library dependencies (first time)
make build       # compile
make program     # flash the attached controller board
```

Defaults come from the `Makefile`: `TARGET=APP_ECE453_FREERTOS`,
`TOOLCHAIN=GCC_ARM`, `CONFIG=Debug`,
`COMPONENTS=FREERTOS HARDFP WICED_BLE bat_client_lib`. The `HARDFP` component
selects hardware floating point, and `bat_client_lib` pulls in the battery
client library.

A serial terminal on the KitProg UART (the console brought up by
`task_console_init`) prints connection state and status, which is useful for
debugging pairing and the CarData link.
