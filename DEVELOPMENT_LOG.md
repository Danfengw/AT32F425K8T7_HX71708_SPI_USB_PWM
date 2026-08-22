# Development Log

### Reference material

- AT32 firmware library examples: `C:\Users\dwang\at32ide-workspace\AT32F425_Firmware_Library_V2.1.4\project\at_start_f425\examples\`
- Schematic: `Desktop\SCH_Schematic2_at32_2026-08-17.pdf`
- Netlist: `Desktop\Netlist_Schematic2_at32_2026-08-17.tel`

## 2026-08-21

### Summary

Reworked the HX71708 load-cell driver from polling to fully interrupt-driven,
redesigned the USB-CDC reporting protocol from request/response to unsolicited
broadcast, and tracked down + fixed a real interrupt-storm lockup that traced
back to unprotected DOUT/PD_SCK lines on the board. Firmware is now stable
under active mechanical load testing on both channels.

### Changes made

**HX71708 driver (`project/src/wk_hx71708.c`, `project/inc/wk_hx71708.h`)**
- Replaced blocking `wk_hx71708_read_raw()`/`wk_hx71708_is_ready()` polling
  with an edge-triggered design:
  - `wk_hx71708_dout_isr(ch)` — called from the DOUT falling-edge EXINT ISR.
    Does the *minimum* possible: mask the channel's own EXINT line, clear the
    pending flag, set a ready hint. No sensor I/O happens in interrupt
    context.
  - `wk_hx71708_get_sample(ch, &value)` — called every main-loop tick.
    Services the ready hint: does the ~50us blocking 25-pulse clock-out
    (`hx71708_shift_in()`) and re-arms the line. This is deliberately kept
    out of ISR context.
  - `wk_hx71708_check_stale(ch)` — periodic watchdog; force-resets a channel
    (datasheet reset pulse) if it hasn't produced a sample within 500ms.
- Fixed a transition bug in `wk_task.c`: on the calibration→normal state
  switch, `ch_raw[]` was still zero-initialized, so the very first tared
  reading came out as `-offset` (a large spurious spike) instead of `0`.
  Now seeded to `ch_offset[]` at the transition.

**EXINT config (`project/src/wk_exint.c`)**
- DOUT_RR (line 1) / DOUT_LL (line 4) switched from rising-edge to
  falling-edge trigger — DOUT idles high and drops low the instant a
  conversion is ready, so falling edge is the correct "ready" signal (the
  old rising-edge config was a leftover no-op from the workbench template).
- DOUT_RR/DOUT_LL GPIO pull changed from `PULL_NONE` to `PULL_UP` at the
  point EXINT is armed, closing a startup race where the line was floating
  with the interrupt already live (before `wk_hx71708_init()` later applied
  its own pull-up).
- EXINT6 (`BTN_1`)/EXINT15 (`MOTOR_EN_SW`) were temporarily disabled during
  the storm debugging (to rule them out as noise sources) and have been
  **re-enabled** — back to their original rising-edge config.

**USB-CDC app layer (`project/src/hx71708_app.c`, `project/inc/hx71708_app.h`)**
- Removed the request/response command protocol (host used to send a 5-byte
  command frame to request a reading).
- Replaced with unsolicited broadcast: `hx71708_broadcast_task()` pushes the
  current status + both channels' tared readings out the CDC port every
  100ms on its own, matching the HX71708's native 10Hz rate. No host polling
  required.

### Root cause: interrupt storm on DOUT lines

Symptom: firmware would run fine for a variable amount of time, then
permanently lock up inside `EXINT1_0_IRQHandler`/`EXINT15_4_IRQHandler`,
taking USB broadcast down with it (confirmed via register inspection:
`SysTick->VAL` still counting → core alive and busy, not hung; `EXINT->intsts`
bit for the affected line staying latched at 1 → real, continuous edges, not
a stuck flag).

Root cause, confirmed via the schematic/netlist
(`~/Desktop/SCH_Schematic2_at32_2026-08-17.pdf` /
`Netlist_Schematic2_at32_2026-08-17.tel`): `DOUT_LL`/`DOUT_RR` and
`PD_SCK_LL`/`PD_SCK_RR` are bare MCU-to-chip traces with **no external pull-up,
no series resistor, no filtering** — and PD_SCK/DOUT are physically adjacent
pins (5/6) on the HX71708 SOP-8 package, making capacitive coupling between
them plausible. Under noise, the DOUT line could re-trigger far faster than a
real 10Hz conversion, and the original ISR design did the full ~50us blocking
clock-out inline, which was enough to starve the main loop entirely once
triggering got fast enough.

Fixes applied, layered:
1. Software: moved the blocking clock-out out of ISR context entirely (see
   driver changes above) — masking on the very first edge means a
   misbehaving line can cost at most one ISR entry per masked episode,
   regardless of how fast/bursty it oscillates.
2. Hardware: added external 10kΩ pull-up resistors directly at the HX71708
   package (DVDD pin 7 → DOUT pin 6) on **both** U15 (LL) and U16 (RR).
3. Confirmed via testing: the fix holds even reconnected through the
   USB hub + long extension cable setup that used to reliably trigger the
   lockup (previously suspected as an additional noise path).

Considered but not applied: a small (~22-100pF) filter cap from DOUT to GND
was discussed as a further hardening option; a 0.1uF ("104") value was
specifically ruled out as far too large — it would low-pass-filter out the
legitimate ~1us data transitions along with any noise.

### Current status

- Both HX71708 channels verified reading correctly under active mechanical
  load (pressed by hand during testing) — smooth, multi-sample ramps
  consistent with real strain-gauge response, independent per channel, no
  lockups.
- USB-CDC broadcast stable, unsolicited, 100ms cadence, both channels.
- EXINT6/EXINT15 (buttons/switch) re-enabled after being temporarily
  disabled for storm isolation.

### Next steps

1. **Power control.** `SW1` + `PWR_EN` gate `U12` (load switch):
   - Press `SW1` → `U12` enables → DC-DC comes up → MCU boots.
   - After GPIO init, MCU pulls `PWR_EN` high itself (self-latch, keeps
     `U12`/power enabled independent of the momentary button).
   - `BTN_1` press detection: short press → reserved for a later phase
     (phase 3, TBD); long press (>2s) → pull `PWR_EN` low → `U12` disables →
     MCU powers off.
2. **UART.** Bring up UART comms.
3. **Motor controller integration test.** Use the UART link to test against
   a real motor controller.

## 2026-08-22

### Summary

Started the power control module and the st7789 display module (spi1+dma).
Both are wired into `main()` but not yet verified on hardware.

### Changes made

**Power control (`project/src/wk_power.c`, `project/inc/wk_power.h`)**
- `wk_power_init()`: latches `PWR_EN` high. Called from `main()` right after
  the other peripheral inits, so `U12`/board power stays enabled once the
  momentary `SW1` press is released.
- `wk_power_task()`: polls `BTN_1` every main loop iteration (no exint
  logic added - `EXINT_LINE_6`'s handler is unchanged, still just clears
  the flag). On a hold >= 2s, releases `PWR_EN` and halts in place (the
  supply rail is expected to collapse shortly after). A press released
  before 2s is detected as a short press but is intentionally a no-op -
  reserved for a later phase per the note above.

**ST7789 display (`project/src/wk_st7789.c` + `.h`, `project/src/st7789_app.c`
+ `.h`, `libraries/st7789/` - LibDriver's generic driver_st7789)**
- `wk_st7789.c` is the hardware port layer: binds driver_st7789's
  spi_init/spi_write_cmd/gpio/delay/debug_print function pointers to
  spi1+dma (`wk_spi1_dma_send()`/`wk_spi1_dma_busy()`, already scaffolded
  by the workbench tool for this exact purpose) and the board's
  reset/rs/cs/backlight gpio pins. `spi_write_cmd` starts a dma tx and
  blocks on `wk_spi1_dma_busy()` until it completes - the driver's
  callback is synchronous by design, so the actual byte clocking is
  offloaded to dma hardware even though the call site waits for it.
- The panel is a 135x240 physical module, run in landscape (240x135) per
  user preference. driver_st7789's high-level draw/fill/clear/write_string
  helpers don't support this panel family's fixed ram offset quirk, so
  `wk_st7789_fill_rect()`/`fill_screen()` are implemented directly on the
  low-level `st7789_set_column_address`/`set_row_address`/`memory_write`/
  `memory_continue_write` primitives, adding the offset themselves.
  `WK_ST7789_COL_OFFSET`/`ROW_OFFSET` (40/53) and `WK_ST7789_MADCTL_VALUE`
  (0x60) are the standard values documented for this panel family in
  landscape - **not yet verified against the actual hardware**; if the
  image comes up shifted or mirrored, adjust these first.
- `st7789_app.c` brings the panel up and fills the screen green as a
  bring-up smoke test (proves the spi/dma link and init sequence without
  depending on text/graphics rendering, which hasn't been built yet).
- Registered `libraries/st7789` in the Eclipse project (`.project` linked
  resources + `.cproject` include paths for both Debug/Release) - it had
  been copied onto disk but was never wired into the build.

**Follow-up: build wiring + button refactor**
- A pre-existing manual copy at `project/AT32_IDE/st7789/` (from before this
  session) had left two on-disk copies of `driver_st7789.*`. Deleted the
  stray copy so `libraries/st7789/` is the single source; also had to
  re-add its `.project` linked-resource entry afterwards, since deleting
  that folder while the IDE held a stale project model dropped it from
  the link list (include path in `.cproject` was untouched, so this only
  showed up as linker errors, not compile errors).
- Pulled BTN_1's press/hold classification out of `wk_power.c` into a
  generic, reusable `wk_button.c`/`.h`: `wk_button_service()` runs a
  debounce/classify state machine (`IDLE -> HELD -> LONG_FIRED`) against
  a `wk_button_type` (gpio pin + `long_press_us` + `on_short_press`/
  `on_long_press` callbacks), firing each callback exactly once per
  hold/release rather than repeatedly. `wk_power.c` now just registers
  `wk_power_off` as BTN_1's `on_long_press` and leaves `on_short_press`
  NULL. Adding another button later is a new `wk_button_type` instance
  serviced the same way - no changes needed to the state machine itself.

**H3 motor-enable squeeze switch (`project/src/wk_motor.c`, `project/inc/wk_motor.h`)**
- `MOTOR_EN_SW` (PA15) is an unlatched momentary switch on H3 (the
  handle/grip trigger) - plain press/release, not a hold-duration
  button, so it's serviced by the same `wk_button_service()` machine as
  BTN_1 but only wires up `on_press`/`on_release` (added to
  `wk_button_type` alongside the existing `on_short_press`/
  `on_long_press`, since BTN_1 needs the latter and H3 needs the
  former - both live on the same generic state machine, each button
  just registers the callbacks it cares about).
- `wk_motor_on_squeeze()`/`wk_motor_on_release()` are placeholders only -
  no actual motor drive yet. Intended behavior once dispatch exists:
  squeezing H3 while the hx71708 has a valid reading enables the motor;
  releasing H3 stops it unconditionally. Left as `/* TODO */` comments
  per the "Motor controller integration test" next step below.
- Decided against EXINT for either BTN_1 or MOTOR_EN_SW: a few ms of
  polling latency is irrelevant for a human-operated switch, and the
  main loop already runs fast enough that there's no debounce benefit
  either. Revisit only if a real low-power (WFI) sleep mode gets added
  later - that's the case where an edge interrupt actually earns its
  keep, since the main loop wouldn't be spinning to catch a polled level.

### Next steps

1. **Verify on hardware.** Confirm the st7789 image is correctly
   positioned/oriented (adjust `WK_ST7789_COL_OFFSET`/`ROW_OFFSET`/
   `MADCTL_VALUE` in `wk_st7789.c` if not) and that `BTN_1` long-press
   actually powers the board off.
2. **UART.** Bring up UART comms.
3. **Motor controller integration test.** Use the UART link to test against
   a real motor controller.
