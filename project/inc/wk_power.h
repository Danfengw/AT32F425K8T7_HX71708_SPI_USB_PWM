/**
  **************************************************************************
  * @file     wk_power.h
  * @brief    board power control: PWR_EN self-latch + BTN_1 long-press
  *           power-off
  **************************************************************************
  */
#ifndef __WK_POWER_H
#define __WK_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f425_wk_config.h"

  /* drive PWR_EN high (latching U12/board power on independent of the
     momentary SW1 press that first brought the board up), and arm BTN_1's
     long-press-to-power-off callback (see wk_button.h). Call once, from
     main(), after wk_gpio_config(). */
  void wk_power_init(void);

  /* service BTN_1's debounce/hold state machine. Call every main loop
     iteration. May not return, if a long press just triggered power-off. */
  void wk_power_task(void);

#ifdef __cplusplus
}
#endif

#endif
