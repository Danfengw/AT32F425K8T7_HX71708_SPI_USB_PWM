/**
  **************************************************************************
  * @file     wk_power.c
  * @brief    board power control: PWR_EN self-latch + BTN_1 long-press
  *           power-off
  *
  * SW1 + PWR_EN gate U12 (load switch) in hardware: pressing SW1 enables
  * U12, which brings up the dc-dc and boots the MCU. From that point on,
  * U12 stays enabled only because wk_power_init() latches PWR_EN high
  * itself - if the MCU never ran this, releasing SW1 would drop power
  * again.
  *
  * BTN_1's press/hold classification is handled by the generic
  * wk_button_service() state machine (wk_button.h); this file just
  * registers what happens on each event - a short press is reserved for
  * a later phase (TBD, see DEVELOPMENT_LOG.md) so on_short_press is left
  * NULL, and on_long_press releases PWR_EN to shut the board down.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wk_power.h"
#include "wk_button.h"

/* BTN_1 must be held continuously this long before PWR_EN is released */
#define WK_POWER_OFF_HOLD_US    (uint32_t)(2000000)

static wk_button_type power_btn;

/**
  * @brief  release PWR_EN so U12 drops board power. Everything past this
  *         point races the supply rail collapsing, so halt here instead
  *         of returning to the caller.
  * @param  none
  * @retval none
  */
static void wk_power_off(void)
{
  gpio_bits_reset(PWR_EN_GPIO_PORT, PWR_EN_PIN);

  while(1)
  {
    __NOP();
  }
}

/**
  * @brief  latch board power on and arm BTN_1's long-press callback.
  * @param  none
  * @retval none
  */
void wk_power_init(void)
{
  gpio_bits_set(PWR_EN_GPIO_PORT, PWR_EN_PIN);

  power_btn.port = BTN_1_GPIO_PORT;
  power_btn.pin = BTN_1_PIN;
  power_btn.long_press_us = WK_POWER_OFF_HOLD_US;
  power_btn.on_press = 0;
  power_btn.on_release = 0;
  power_btn.on_short_press = 0;
  power_btn.on_long_press = wk_power_off;
  power_btn.state = WK_BUTTON_IDLE;
  power_btn.press_ticks = 0;
}

/**
  * @brief  service BTN_1's debounce/hold state machine.
  * @param  none
  * @retval none
  */
void wk_power_task(void)
{
  wk_button_service(&power_btn);
}
