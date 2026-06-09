# RC Car — Vehicle Firmware (`ece453-mtb-freertos-app-Car`)

Firmware for the **car** half of the ECE 453 BLE remote-controlled car. This is
the moving vehicle: it drives the DC motors and steering servo, reads its
onboard sensors, and reports status back to the handheld controller over
Bluetooth Low Energy.

In BLE terms the car is the **GATT server / peripheral**. It advertises a custom
**CarData** service, accepts drive commands written by the controller
(`CarDataIn`), and pushes a status byte back as notifications (`CarDataOut`).

> This project is part of a two-board system. See the
> [top-level README](../README.md) for the overall architecture and the link
> protocol, and the [controller README](../ece453-mtb-freertos-app-Controller)
> for the other half.

## Platform

- **MCU / board:** Infineon PSoC&trade; 6 (CAT1A), board target
  `APP_ECE453_FREERTOS`
- **RTOS:** FreeRTOS
- **BLE stack:** AIROC&trade; / WICED BLE (`COMPONENTS = FREERTOS WICED_BLE`)
- **Toolchain:** `GCC_ARM`, `CONFIG = Debug` (defaults in the `Makefile`)

## How it boots

`main.c` performs a fixed startup sequence:

1. `cybsp_init()` — initialize the board and peripherals.
2. `task_console_init()` — bring up the UART console so `task_print*` works
   before BLE starts.
3. `cybt_platform_config_init()` — configure the BT platform.
4. `app_kvstore_bd_config()` — set up the flash block device used by kv-store
   for bonding storage.
5. `wiced_bt_stack_init(app_bt_management_callback, ...)` — start the BLE stack;
   application tasks are created from the stack/GATT callbacks as the connection
   comes up.
6. `vTaskStartScheduler()` — hand control to FreeRTOS.

## Source layout

```
ece453-mtb-freertos-app-Car/
├── main.c / main.h            ← startup, includes, BLE stack init
├── FreeRTOSConfig.h           ← FreeRTOS tuning
├── Makefile                   ← TARGET / TOOLCHAIN / COMPONENTS
├── source/
│   ├── FreeRTOS_CLI.[ch]      ← command-line interpreter used by the console
│   ├── app_hw/                ← hardware + FreeRTOS task layer (see below)
│   ├── app_bt/                ← BLE event handling, GATT, bonding
│   └── vl53l3cx/              ← ST VL53L3CX time-of-flight sensor driver
└── bsps/ + libs/ + deps/      ← ModusToolbox BSP, libraries, dependencies
```

### `source/app_hw/` — hardware and tasks

| File(s) | Responsibility |
| --- | --- |
| `task_dc_motor.[ch]` | Drive the DC motors (throttle / direction from the control byte) |
| `task_servo.[ch]` | Steering servo position (left/right from the control byte) |
| `task_tof.[ch]` | VL53L3CX time-of-flight distance; feeds the autobrake bit |
| `task_ltr390uv.[ch]` | LTR-390UV ambient-light / UV sensor; feeds the light bit |
| `task_acc.[ch]`, `lsm6dsm_reg.[ch]` | LSM6DSM accelerometer (reserved in the packet) |
| `task_sendCarPacket.[ch]` | Assembles the 1-byte status packet and pushes it to `CarDataOut` |
| `task_io_expander.[ch]` | I/O expander control |
| `audio_amp.[ch]` | Audio amplifier (horn / sound output) |
| `task_blink.[ch]` | Heartbeat / status LED |
| `task_console.[ch]` | UART console + CLI plumbing |
| `i2c.[ch]`, `spi.[ch]` | I2C / SPI bus helpers shared by the sensor tasks |
| `app_serial_flash.c`, `app_internal_aux_flash.c`, `app_flash_common.h` | Flash access for bonding storage |
| `ece453_pins.h` | Board pin assignments |

### `source/app_bt/` — Bluetooth layer

| File(s) | Responsibility |
| --- | --- |
| `app_bt_event_handler.[ch]` | BLE management/connection events, advertising |
| `app_bt_gatt_handler.[ch]` | GATT read/write/notify handling for the CarData service |
| `app_bt_bonding.[ch]` | Persistent bonding (pairing keys in flash) |
| `app_bt_utils.[ch]` | Logging / enum-to-string helpers |

## BLE protocol (this side)

### Inbound — `CarDataIn` (controller writes, car receives)

One byte, one bit per control event. The car decodes it to drive the motors and
servo:

| Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Field | gear_sel | turn_right | turn_left | brake | accelerate | reset | pair | horn |

The bits the drive logic acts on are `accelerate`, `turn_left`, `turn_right`,
and `gear_sel` (0 = forward, 1 = reverse).

### Outbound — `CarDataOut` (car notifies, controller receives)

`task_sendCarPacket` runs every **500 ms** (`CARPACKET_PERIOD_MS`). It requests
fresh readings from the sensor tasks via their queues, waits on
`q_carpacket_report` for the responses, assembles a single status byte, prints
it to the console, and writes it into the `CarDataOut` characteristic so the
stack notifies the connected client:

| Bits | 7 | 6 | 5:0 |
| --- | --- | --- | --- |
| Field | autobrake | light | speed (0–63) |

- **autobrake (bit 7):** set when the ToF distance is below
  `CARPACKET_AUTOBRAKE_MM` (400 mm).
- **light (bit 6):** set when the LTR-390UV reading is above its threshold.
- **bits 5:0:** speed; the accelerometer is currently unused and these are
  filled as reserved.

## Building and flashing

```bash
make getlibs     # fetch library dependencies (first time)
make build       # compile
make program     # flash the attached car board
```

Defaults come from the `Makefile`: `TARGET=APP_ECE453_FREERTOS`,
`TOOLCHAIN=GCC_ARM`, `CONFIG=Debug`, `COMPONENTS=FREERTOS WICED_BLE`. Override on
the command line if needed, e.g. `make build CONFIG=Release`.

A serial terminal on the KitProg UART (the console brought up by
`task_console_init`) prints status, including each assembled CarDataOut byte,
which is useful for debugging the sensor pipeline.
