/**
  **************************************************************************
  * @file     wk_button.c
  * @brief    generic debounced button state machine, dispatched through
  *           per-button callbacks (short/long press) rather than
  *           hardcoded per-button logic
  *
  * IDLE -> (pressed) -> HELD -> (long_press_us reached) -> LONG_FIRED -> (released) -> IDLE
  *                            -> (released first)        -> IDLE, on_short_press fires
  *
  * on_press/on_release fire on every press/release regardless of hold
  * duration - use these for a plain momentary switch that just needs to
  * gate something for as long as it's held (e.g. a motor-enable
  * trigger). on_long_press fires exactly once per hold, right when the
  * threshold is crossed (LONG_FIRED suppresses repeat firing for the
  * rest of that hold) - callers that want an action once at the
  * threshold (e.g. a power-off) get that directly, with no need to guard
  * against being re-invoked every poll for as long as the button stays
  * held.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wk_button.h"
#include "wk_system.h"

/**
  * @brief  run one step of btn's debounce/classify state machine.
  * @param  btn: button to service
  * @retval none
  */
void wk_button_service(wk_button_type *btn)
{
  uint8_t pressed = gpio_input_data_bit_read(btn->port, btn->pin) ? 1 : 0;

  switch(btn->state)
  {
    case WK_BUTTON_IDLE:
      if(pressed)
      {
        btn->state = WK_BUTTON_HELD;
        /* seed the reference tick to "now" so the check below measures
           hold duration from this instant */
        (void)wk_tick_elapsed_us(&btn->press_ticks, 0);
        if(btn->on_press)
        {
          btn->on_press();
        }
      }
      break;

    case WK_BUTTON_HELD:
      if(!pressed)
      {
        btn->state = WK_BUTTON_IDLE;
        if(btn->on_release)
        {
          btn->on_release();
        }
        if(btn->on_short_press)
        {
          btn->on_short_press();
        }
      }
      else if(wk_tick_elapsed_us(&btn->press_ticks, btn->long_press_us))
      {
        btn->state = WK_BUTTON_LONG_FIRED;
        if(btn->on_long_press)
        {
          btn->on_long_press();
        }
      }
      break;

    case WK_BUTTON_LONG_FIRED:
      if(!pressed)
      {
        btn->state = WK_BUTTON_IDLE;
        if(btn->on_release)
        {
          btn->on_release();
        }
      }
      break;

    default:
      break;
  }
}
