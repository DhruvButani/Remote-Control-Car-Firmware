# RC Car — ECE 453 BLE Remote-Controlled Car

This repository bundles the two firmware projects that make up a Bluetooth Low
Energy remote-controlled car built for ECE 453. Both projects target an
Infineon PSoC&trade; 6 (CYW20829-class / CAT1A) board running FreeRTOS and the
AIROC&trade; (WICED) BLE stack under ModusToolbox&trade;.

The system is split across two physically separate boards that talk to each
other over a single BLE connection:

| Subdirectory | Role | BLE role | What it does |
| --- | --- | --- | --- |
| [`ece453-mtb-freertos-app-Car`](./ece453-mtb-freertos-app-Car) | The vehicle | GATT **server** (peripheral) | Drives the motors/servo, reads onboard sensors, and reports status back to the controller |
| [`ece453-mtb-freertos-app-Controller`](./ece453-mtb-freertos-app-Controller) | The handheld remote | GATT **client** (central) | Reads the joystick/buttons, sends drive commands, and shows car status on an LCD |

Each subdirectory has its own detailed README covering its tasks, hardware, and
BLE characteristics.

## System architecture

```
        Controller board                         Car board
  ┌──────────────────────────┐            ┌──────────────────────────┐
  │  Joystick / buttons / mic │            │  DC motors / steering servo│
  │            │              │            │            ▲             │
  │      Controller_Events    │            │      drive decode        │
  │            │              │            │            │             │
  │   task_bt_send (timer)    │  CarDataIn │  app_bt_gatt_handler     │
  │            │ ───────────────────────────▶  (write)                │
  │   CarData GATT client     │            │  CarData GATT server     │
  │            ▲ ───────────────────────────  task_sendCarPacket      │
  │            │              │ CarDataOut │            ▲             │
  │   task_lcd (SSD1351)      │ (notify)   │  ToF / UV / accel tasks  │
  └──────────────────────────┘            └──────────────────────────┘
```

The link carries two one-byte payloads on a custom **CarData** GATT service:

**CarDataIn** — Controller → Car (write). One bit per control event:

| Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Field | gear_sel | turn_right | turn_left | brake | accelerate | reset | pair | horn |

`gear_sel` selects forward (0) or reverse (1); `accelerate`, `turn_left`,
`turn_right`, and `gear_sel` are the bits the car actually acts on for driving.

**CarDataOut** — Car → Controller (notification). One status byte:

| Bits | 7 | 6 | 5:0 |
| --- | --- | --- | --- |
| Field | autobrake | light (UV) | speed (0–63) |

`autobrake` is set when the time-of-flight sensor sees an obstacle closer than
400 mm; `light` reflects the LTR-390UV ambient-light sensor crossing its
threshold. The controller parses this byte and renders it on the LCD.

## Repository layout

```
RC Car/
├── README.md                          ← you are here
├── ece453-mtb-freertos-app-Car/       ← vehicle firmware (GATT server)
│   ├── main.c / main.h
│   ├── source/app_hw/                 ← motor, servo, ToF, UV, accel, I2C/SPI tasks
│   ├── source/app_bt/                 ← BLE event + GATT handlers, bonding
│   ├── source/vl53l3cx/               ← ST VL53L3CX time-of-flight driver
│   └── bsps/ + Makefile               ← ModusToolbox BSP and build config
└── ece453-mtb-freertos-app-Controller/← remote firmware (GATT client)
    ├── main.c / main.h
    ├── source/app_hw/                 ← joystick, buttons, mic, LCD (SSD1351) tasks
    ├── source/app_bt/                 ← BLE client (CarData + battery), bonding
    └── bsps/ + Makefile               ← ModusToolbox BSP and build config
```

## Building and flashing

Both projects are standard ModusToolbox&trade; applications and are built the
same way. From inside **either** subdirectory:

```bash
# install/refresh library dependencies
make getlibs

# build (defaults: TARGET=APP_ECE453_FREERTOS, TOOLCHAIN=GCC_ARM, CONFIG=Debug)
make build

# build and flash the attached board
make program
```

You can also open each subdirectory as a separate application in the
ModusToolbox&trade; Eclipse IDE / VS Code extension and use the generated launch
configurations. The shared build settings live in each project's `Makefile`
(`TARGET`, `TOOLCHAIN`, `COMPONENTS`, etc.).

> **Flash both boards.** Program the car board with the `...-Car` project and
> the remote board with the `...-Controller` project. They are not
> interchangeable — only the car advertises the CarData service, and only the
> controller scans/connects to it.

## Pairing and bonding

Both sides implement persistent bonding backed by the kv-store / serial-flash
libraries, so a paired car and controller reconnect automatically after a power
cycle. On the controller, the **pair** button (SW2) drives the connect/bond
flow; bond data is stored in flash via `app_bt_bonding`.

## Prerequisites

- ModusToolbox&trade; (with the GCC_ARM toolchain that ships with it)
- A PSoC&trade; 6 / AIROC&trade; BLE board for each side, with the ECE 453
  carrier hardware (motor driver, servo, sensors on the car; joystick, buttons,
  and SSD1351 OLED on the controller)
- A USB connection per board for programming and the serial console (KitProg)

See the per-project READMEs for the full task list, pin assignments, and BLE
detail of each side.
