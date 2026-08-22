/**
  **************************************************************************
  * @file     wk_button.h
  * @brief    generic debounced button state machine, dispatched through
  *           per-button callbacks (short/long press) rather than
  *           hardcoded per-button logic
  **************************************************************************
  */
#ifndef __WK_BUTTON_H
#define __WK_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f425_wk_config.h"

typedef void (*wk_button_callback_type)(void);

typedef enum
{
  WK_BUTTON_IDLE      = 0,        /* not pressed */
  WK_BUTTON_HELD      = 1,        /* pressed, long_press_us not reached yet */
  WK_BUTTON_LONG_FIRED = 2,       /* pressed, on_long_press already fired this hold */
} wk_button_state_type;

/**
  * @brief  one button's wiring + config + live state. Zero-initialize (or
  *         explicitly set state = WK_BUTTON_IDLE, press_ticks = 0) before
  *         the first wk_button_service() call. Any callback may be NULL
  *         if that event isn't used - a plain momentary switch (e.g. a
  *         motor-enable trigger) only needs on_press/on_release, while a
  *         press/hold button (e.g. a power button) only needs
  *         on_short_press/on_long_press.
  */
typedef struct
{
  gpio_type *port;
  uint16_t   pin;
  uint32_t   long_press_us;

  wk_button_callback_type on_press;          /* fires once, the instant a press is detected */
  wk_button_callback_type on_release;        /* fires once, the instant a release is detected */
  wk_button_callback_type on_short_press;    /* fires once, on release, if never held long enough */
  wk_button_callback_type on_long_press;     /* fires once, the instant long_press_us is reached */

  wk_button_state_type state;
  uint32_t   press_ticks;
} wk_button_type;

  /* poll btn, debounce/classify its hold duration against
     btn->long_press_us, and fire whichever callback applies (at most one
     per call). call every main loop iteration for each button in use. */
  void wk_button_service(wk_button_type *btn);

#ifdef __cplusplus
}
#endif

#endif
