# RC Car — Task & BLE Reference

Two CY8CPROTO-063-BLE boards run as a paired RC car system over BLE:

- **Controller** — handheld unit (BLE Central / GATT Client). Buttons, joystick, mic, OLED display.
- **Car** — vehicle unit (BLE Peripheral / GATT Server). VL53L3CX ToF, LTR-390UV light, LSM6DSM accelerometer, DC motor, servo, LCD-bound state.

Below is every FreeRTOS task on each board (plus the timer ISRs and BLE callbacks that act like tasks), what it does, and how it connects to everything else (queue, notification, semaphore, BLE characteristic, or shared global).

---

## Controller Tasks

### `button_task`
- **File:** `source/app_bt/battery_client.c`
- **What it does:** Handles presses of the on-board user button (`CYBSP_USER_BTN`). Sleeps on a task notification; on a button release it calls `battery_client_enter_pairing()` to start a BLE scan for the Car server.
- **Connection:** GPIO ISR `gpio_interrupt_handler` calls `vTaskNotifyGiveFromISR(button_handle, ...)`. The task then drives the BLE stack via `wiced_bt_ble_observe()`.

### `task_blink`
- **File:** `source/app_hw/task_blink.c`
- **What it does:** Toggles the status LED on `PIN_LED`. A FreeRTOS software timer flips the GPIO every 500 ms; the task itself just starts/stops the timer based on incoming commands.
- **Connection:** Queue `q_blink` (depth 1, type `blink_message_type_t`). Producer is the `blink` CLI command in `cli_handler_blink`.

### `task_io_expander`
- **File:** `source/app_hw/task_io_expander.c`
- **What it does:** Marshals reads/writes to the TCA9534 I/O expander over the shared I²C bus. Read requests carry a return queue so callers can wait synchronously for the response.
- **Connection:** `q_io_expander_req` (request queue), `q_io_expander_rsp` (default response queue), `Semaphore_I2C` mutex around the I²C transactions. CLI command `ioxp` is a producer.

### `task_console_tx`
- **File:** `source/app_hw/task_console.c`
- **What it does:** Single-writer for the debug UART. Pulls `debug_message_data_t` off the queue and prints them with severity prefixes (none / info / warning / error).
- **Connection:** Queue `q_console_tx`. Any task can produce via the `task_print*` macros. *Currently disabled in `main.c` so SCB5 (P5_0/P5_1) can be used for the OLED's SPI.*

### `task_console_rx`
- **File:** `source/app_hw/task_console.c`
- **What it does:** Console input + FreeRTOS-CLI parser. Sleeps on a task notification from the UART RX ISR (`console_event_handler`); on Enter it runs `FreeRTOS_CLIProcessCommand` over the buffered line.
- **Connection:** Task notification from UART ISR; output goes back through `task_console_tx`.

### `bt_send_task`
- **File:** `source/app_hw/task_bt_send.c`
- **What it does:** Periodic BLE uplink. A 200 ms hardware timer ISR notifies it; on each tick it packs the eight `Controller_Events` bits into one byte and writes it to the Car's **CarDataIn** characteristic (`car_data_send_control` → `wiced_bt_gatt_client_send_write` with `GATT_CMD_WRITE`).
- **Connection:** Timer ISR `bt_send_timer_isr` → `vTaskNotifyGiveFromISR`; BLE Write Without Response over the active connection (`battery_client_app_data.conn_id`, `car_data_in_get_val_handle()`).

### `button_timer_handler` (ISR, not a task)
- **File:** `source/app_hw/buttons.c`
- **What it does:** Hardware timer ISR (5 ms) that polls the controller button port and debounces HORN, PAIR, RESET, ACCELERATE, BRAKE, GEAR_SEL, and the joystick record button. A press has to be present for 5 consecutive samples before the corresponding `Controller_Events` bit is asserted.
- **Connection:** Writes directly into the global `Controller_Events` bitfield. `bt_send_task` later samples the same struct.

### `joy_timer_handler` (ISR, not a task)
- **File:** `source/app_hw/joystick.c`
- **What it does:** Hardware timer ISR (5 ms) that reads the X/Y ADC channels, classifies the position via `joystick_get_pos()`, and updates `Controller_Events.turn_left` / `Controller_Events.turn_right`.
- **Connection:** Writes the same shared `Controller_Events` struct.

### LCD bring-up (`vLCDTask_init`)
- **File:** `source/app_hw/task_lcd.c`
- **What it does:** Brings up the SSD1351 OLED on SCB5 SPI and currently just paints a circle to verify wiring. The intent (per `task_bt_packet.c` and `car_data_process_notification`) is for the LCD to be redrawn with **speed**, **light status**, and **autobrake status** every time a CarDataOut notification lands.
- **Connection:** State setters `update_vehicle_speed`, `update_horn_status`, `update_auto_brake_status`, `update_headlights_status` (in `task_bt_packet.c`) populate a static `core_state` (`lcd_info_t`); `get_lcd_info()` reads it for the LCD draw routine. The setters are invoked from the BLE notification handler.

### BLE client / GATT (`battery_client.c` + `car_data_client.c`)
- **What it does:** Not a FreeRTOS task — the WICED BT stack drives the callback `battery_client_gatts_callback` (and the `wiced_bt_battery_client_callback` shim). Together they handle scan results, connect/disconnect, custom 128-bit UUID matching for CarData service, characteristic discovery (CarDataOut/CarDataIn val_handles), CCCD descriptor discovery, notification subscription, and incoming notifications. On a CarDataOut notification, `car_data_process_notification` unpacks the byte and (per design) feeds the LCD setters.
- **Connection:** BLE radio events → stack thread → callback. Outputs cross task boundaries by writing into the shared `core_state` struct or by issuing more BLE writes via `car_data_send_control`.

---

## Car Tasks

### `task_blink`
- **File:** `source/app_hw/task_blink.c`
- **What it does:** Identical to the Controller's status LED task — software timer plus queue-driven start/stop.
- **Connection:** Queue `q_blink`, blink CLI command.

### `task_console_tx` / `task_console_rx`
- **File:** `source/app_hw/task_console.c`
- **What it does:** Same UART debug console + FreeRTOS-CLI parser as the Controller.
- **Connection:** Queue `q_console_tx` for prints; UART RX ISR notification for input lines.

### `task_tof`
- **File:** `source/app_hw/task_tof.c`
- **What it does:** Driver task for the VL53L3CX time-of-flight sensor. Blocks on its queue for a `TOF_READ` (CLI) or `TOF_READ_PACKET` (periodic) request, polls `VL53LX_GetMeasurementDataReady`, reads multi-ranging data, and either prints the distance or posts a `packet_sensor_report_t {PKT_SENSOR_TOF, distance_mm}`.
- **Connection:** In via `q_tof`, out via `q_carpacket_report`. CLI command `tof` is a producer.

### `task_ltr390uv`
- **File:** `source/app_hw/task_ltr390uv.c`
- **What it does:** Driver task for the LTR-390UV-01 ambient/UV light sensor on the shared I²C bus. Polls MAIN_STATUS for data-ready, reads three ALS data registers, assembles the 20-bit count, and either prints `Light: ON / OFF` (vs `LTR390UV_LUX_THRESHOLD_COUNTS`) or posts a `packet_sensor_report_t {PKT_SENSOR_LTR390UV, als_count}`.
- **Connection:** In via `q_ltr390uv`, out via `q_carpacket_report`. Bus is shared with `task_io_expander`, gated by `Semaphore_I2C`.

### `task_acc`
- **File:** `source/app_hw/task_acc.c`
- **What it does:** Driver task for the LSM6DSM accelerometer on its **own** dedicated I²C bus (`acc_i2c_obj` — no mutex needed because no other task touches that SCB). Reads X/Y/Z, converts to mg, computes vector magnitude, quantizes to a 6-bit code, and either prints or posts a report.
- **Connection:** In via `q_acc`, out via `q_carpacket_report`.

### `task_sendCarPacket`
- **File:** `source/app_hw/task_sendCarPacket.c`
- **What it does:** Aggregator/uplink. Every `CARPACKET_PERIOD_MS` it fires `*_READ_PACKET` requests at the LTR-390UV and ACC tasks (ToF is currently commented out in the request loop), drains responses from the report queue until both arrive (or the deadline elapses), packs them plus the ToF-derived auto-brake bit into one byte, writes it into `app_cardata_cardataout[0]`, and calls `app_bt_send_message()` to push a BLE notification.
- **Connection:** Out → `q_ltr390uv` / `q_acc`. In ← `q_carpacket_report`. Then BLE notification on **CarDataOut** (`HDLC_CARDATA_CARDATAOUT_VALUE`).

### `task_io_expander`
- **File:** `source/app_hw/task_io_expander.c`
- **What it does:** Same TCA9534 helper task as on the Controller.
- **Connection:** `q_io_expander_req` / `q_io_expander_rsp`, `Semaphore_I2C`.

### `task_servo` (helper, not an actual task)
- **File:** `source/app_hw/task_servo.c`
- **What it does:** `task_servo_init()` brings up TCPWM at 50 Hz parked at the 1.5 ms neutral pulse. `task_servo_set_pulse(SERVO_POS_LEFT|NEUTRAL|RIGHT)` rewrites the duty.
- **Connection:** Called synchronously. Currently not wired into the GATT-write path; the intended hook is the `turn_left`/`turn_right` bits from CarDataIn.

### `task_dc_motor` (helper, not an actual task)
- **File:** `source/app_hw/task_dc_motor.c`
- **What it does:** Init / `set_direction` / `set_speed` / `stop` for the active-low PWM drive plus a direction GPIO (`MOTOR_DIR_PIN`).
- **Connection:** Invoked synchronously from `app_bt_set_value()` in `app_bt_gatt_handler.c` when CarDataIn is written — the `accelerate` bit toggles the motor on/off today.

### BLE management (`app_bt_event_handler.c`)
- **What it does:** `app_bt_management_callback` handles the WICED BT stack lifecycle: `BTM_ENABLED_EVT` runs `app_bt_application_init()` (advertising start), pairing IO-cap negotiation, link-key save/restore via the bonding module, encryption status, and adv-state changes.
- **Connection:** Stack thread callback only. Populates the global `car_sensor_state` (conn_id, remote_addr, indication-pending flag) used elsewhere.

### BLE GATT server (`app_bt_gatt_handler.c`)
- **What it does:** `app_bt_gatt_callback` dispatches GATT events:
  - **Connect / disconnect** — sets `car_sensor_state.conn_id`; restarts undirected advertising on disconnect.
  - **Reads / writes / MTU / read-by-type** — handled via the auto-generated attribute table.
  - **Write to CarDataIn** (`HDLC_CARDATA_CARDATAIN_VALUE`) — the inbound seam from the Controller. The 1-byte payload is unpacked, logged, and (for now) bit 3 (`accelerate`) is mapped to `task_dc_motor_set_speed` / `_stop`.
  - **Write to CarDataOut CCCD** (`HDLD_CARDATA_CARDATAOUT_CLIENT_CHAR_CONFIG`) — enables/disables notifications and persists the bond's CCCD state via `app_bt_update_cccd`.
  - **`app_bt_send_message`** — sends the latest `app_cardata_cardataout` byte as a notification (or indication, if that's what the peer subscribed to). Called by `task_sendCarPacket`.
- **Connection:** Stack-thread callback. Crosses into application code via the shared GATT attribute buffers (`app_cardata_cardatain`, `app_cardata_cardataout`) and direct calls into the motor helpers.

---

## BLE Characteristics — Reference

### Service: `CarData`
- **UUID:** `FF202D10-8CCC-425A-9FE3-76B086A5C57F`
- Server: handles `0x000A`–`0x000F` (auto-generated by the Bluetooth Configurator).

### Characteristic: `CarDataOut` — Server → Client (Car → Controller)
- **UUID:** `6E5500D0-D02C-4754-A79C-620254558AE7`
- **Properties:** Read, Notify
- **Server handle:** `HDLC_CARDATA_CARDATAOUT_VALUE` = `0x000C`
- **CCCD handle:** `HDLD_CARDATA_CARDATAOUT_CLIENT_CHAR_CONFIG` = `0x000D`
- **Server variable:** `app_cardata_cardataout[]`
- **Discovered val_handle on client:** `0x000C`
- **Producer:** `task_sendCarPacket` packs the byte; `app_bt_send_message` ships it as a notification.
- **Consumer:** Controller's GATT-notification handler (`battery_client_gatts_callback` → `car_data_process_notification`).

### Characteristic: `CarDataIn` — Client → Server (Controller → Car)
- **UUID:** `F48780E6-9CED-4B29-A6B1-0EEF3DB832BD`
- **Properties:** Write, Write Without Response
- **Server handle:** `HDLC_CARDATA_CARDATAIN_VALUE` = `0x000F`
- **Server variable:** `app_cardata_cardatain[]`
- **Discovered val_handle on client:** `0x000F`
- **Producer:** `bt_send_task` on the Controller (`car_data_send_control` → `wiced_bt_gatt_client_send_write` with `GATT_CMD_WRITE` for low latency).
- **Consumer:** Car's GATT write handler (`app_bt_gatt_req_write_handler` → `app_bt_set_value`).

```
Service: CarData
├── Characteristic: CarDataOut (uint8)        Server -> Client (Notify)
│   ├── Value handle: 0x000C
│   └── CCCD handle:  0x000D
└── Characteristic: CarDataIn  (uint8)        Client -> Server (Write / WWR)
    └── Value handle: 0x000F
```

---

## Bit Decoding

Both directions use a single packed byte. Always sent as one `uint8_t`, MSB → bit 7.

### Controller → Car (`CarDataIn`, 1 byte)

Packed in `task_bt_send.c::bt_send_task`:

```
  bit 7    bit 6      bit 5     bit 4    bit 3       bit 2    bit 1   bit 0
[gear_sel][turn_right][turn_left][brake][accelerate][reset][pair][horn]
```

| Bit | Field        | Source on Controller                                     | Notes |
|-----|--------------|----------------------------------------------------------|-------|
| 0   | `horn`       | `Controller_Events.horn` (button debouncer)              | 1 = pressed |
| 1   | `pair`       | `Controller_Events.pair` (button debouncer)              | 1 = pressed |
| 2   | `reset`      | `Controller_Events.reset` (button debouncer)             | 1 = pressed |
| 3   | `accelerate` | `Controller_Events.accelerate` (trigger debouncer)       | 1 = pulled |
| 4   | `brake`      | hard-coded `0` in current `bt_send_task`                 | Wire `Controller_Events.brake` here when ready. |
| 5   | `turn_left`  | `Controller_Events.turn_left` (joystick ISR)             | 1 = stick left |
| 6   | `turn_right` | `Controller_Events.turn_right` (joystick ISR)            | 1 = stick right |
| 7   | `gear_sel`   | `Controller_Events.gear_sel` (gear-select switch)        | `0` = REV, `1` = FWD (per `cli_cmd_status`) |

Decoded on the Car in `app_bt_gatt_handler.c::app_bt_set_value` under `case HDLC_CARDATA_CARDATAIN_VALUE`:

```c
uint8_t ev = app_cardata_cardatain[0];
bool horn       = (ev >> 0) & 1;
bool pair       = (ev >> 1) & 1;
bool reset      = (ev >> 2) & 1;
bool accelerate = (ev >> 3) & 1;
bool brake      = (ev >> 4) & 1;
bool turn_left  = (ev >> 5) & 1;
bool turn_right = (ev >> 6) & 1;
bool gear_sel   = (ev >> 7) & 1;
```

Today only `accelerate` drives `task_dc_motor_set_speed(...)` / `task_dc_motor_stop()`; the rest are logged.

### Car → Controller (`CarDataOut`, 1 byte) — **layout mismatch, see note below**

What the **Car packs** in `task_sendCarPacket.c::task_sendCarPacket`:

```
  bit 7    bit 6        bits 5..0
[ uv_on ][autobrake][acc6 (0..63)]
```

```c
uint8_t packet = acc6;                        /* bits 5:0 = accelerometer magnitude code */
packet |= ((autobrake & 0x01U) << 6);         /* bit 6   = TOF-derived autobrake */
packet |= ((uv_on     & 0x01U) << 7);         /* bit 7   = LTR-390UV ambient light */
```

What the **Controller decodes** in `car_data_client.c::car_data_process_notification`:

```c
parsed.speed     = val & 0x3F;          /* bits 5:0 */
parsed.light     = (val >> 6) & 0x01;   /* bit 6    */
parsed.autobrake = (val >> 7) & 0x01;   /* bit 7    */
```

> **Mismatch — pick one and align both sides.**
>
> The Car puts `autobrake` on bit 6 and `uv_on` (light) on bit 7. The Controller currently treats bit 6 as `light` and bit 7 as `autobrake`, so those two flags will appear swapped on the LCD until one side is corrected.
>
> The Car never computes a true vehicle speed — bits 5..0 are an accelerometer-magnitude code (`mag_mg / ACC_PACKET_MG_PER_LSB`, capped at 63). The Controller labels the same bits `speed`, which is fine semantically as long as everyone understands the field.
>
> Recommended canonical layout (matches the LCD spec — speed / light / autobrake):
>
> ```
>   bit 7        bit 6     bits 5..0
> [autobrake] [light]    [speed (0..63)]
> ```
>
> To adopt it, swap the two shifts in `task_sendCarPacket`:
>
> ```c
> packet |= ((uv_on     & 0x01U) << 6);   /* light     -> bit 6 */
> packet |= ((autobrake & 0x01U) << 7);   /* autobrake -> bit 7 */
> ```
>
> The Controller's decode is already in this form, so no client change would be needed.

| Bit(s) | Field (canonical) | Car source                                                                                | Controller use |
|--------|-------------------|-------------------------------------------------------------------------------------------|----------------|
| 5..0   | `speed`           | `task_acc` magnitude code (or, eventually, real wheel speed)                              | LCD speed readout |
| 6      | `light`           | `task_ltr390uv` — `als_count > LTR390UV_LUX_THRESHOLD_COUNTS`                             | LCD light icon |
| 7      | `autobrake`       | `task_sendCarPacket` — `last_distance_mm < CARPACKET_AUTOBRAKE_MM` (from `task_tof`)      | LCD auto-brake icon |

---

## End-to-End Feedback Loop

Putting the BLE characteristics, queues, and tasks together:

```
┌────────────────────────────  CONTROLLER  ───────────────────────────┐
│                                                                      │
│  buttons.c (timer ISR) ─┐                                            │
│  joystick.c (timer ISR) ┼──> Controller_Events (shared bitfield)     │
│                         │            │                               │
│  bt_send_timer (200 ms) ┴──notify──> bt_send_task ──pack 1 byte──┐   │
│                                                                  │   │
│                                                                  ▼   │
│                                                  car_data_send_control()
│                                                  GATT_CMD_WRITE @ 0x000F  ──BLE──┐
│                                                                                  │
│  battery_client_gatts_callback (BLE stack thread)                                │
│       └─ car_data_process_notification  <──notification @ 0x000C──┐              │
│              speed / light / autobrake                            │              │
│                          │                                        │              │
│   update_vehicle_speed / update_*_status (task_bt_packet.c)       │              │
│                          ▼                                        │              │
│                   core_state (lcd_info_t)                         │              │
│                          ▼                                        │              │
│                    LCD redraw (SSD1351)                           │              │
└───────────────────────────────────────────────────────────────────│──────────────┘
                                                                    │              │
                                                                    │              │
┌───────────────────────────────  CAR  ─────────────────────────────│──────────────┘
│                                                                   │
│  app_bt_gatt_callback → app_bt_set_value @ 0x000F  <──write────── │
│       └─ unpack ev byte; ev[3] -> task_dc_motor_set_speed/stop    │
│                                                                   │
│  task_sendCarPacket (period CARPACKET_PERIOD_MS):                 │
│        send LTR390UV_READ_PACKET ─────> q_ltr390uv ──> task_ltr390uv
│        send ACC_READ_PACKET ──────────> q_acc       ──> task_acc
│        (TOF read currently latched/cached)                        │
│        drain q_carpacket_report (last_als_count,                  │
│                                   last_distance_mm, last_acc_code)│
│        pack 1 byte (acc6 | autobrake<<6 | uv_on<<7)               │
│        app_cardata_cardataout[0] = packet                         │
│        app_bt_send_message() ──notification @ 0x000C──────────────┘
│                                                                    
└────────────────────────────────────────────────────────────────────
```

---

## Notes / Cleanup To-Dos

1. **CarDataOut bit-6 / bit-7 swap.** Decide on a canonical layout (`autobrake` on bit 7, `light` on bit 6 is the cleanest match to both the LCD spec and the Controller's existing decode) and align `task_sendCarPacket`.
2. **`buttons.c` and `joystick.c` are ISRs, not tasks.** They update `Controller_Events` from interrupt context — no queue. Worth labeling them as "ISR-driven event sources" in any architecture diagram.
3. **`task_servo.c` and `task_dc_motor.c` on the Car** are init/setter helpers, not real tasks. The motor is driven synchronously from the GATT write callback today; the servo isn't wired into that path yet.
4. **`task_bt_send.c` brake bit is hard-coded to 0.** When the brake control loop is finalized, swap in `Controller_Events.brake`.
5. **`task_console_init()` is currently disabled in `Controller/main.c`** so SCB5 can be repurposed for the OLED's SPI. `task_print*` calls become no-ops while it's gated off — re-enable once SPI is moved to a different SCB.
6. **ToF read in `task_sendCarPacket`** is commented out; only the cached `last_distance_mm` is used. Re-enable once the VL53L3CX timing budget plays nicely with the period.
