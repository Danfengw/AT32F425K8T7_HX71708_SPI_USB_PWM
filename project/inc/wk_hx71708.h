/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __WK_HX71708_H
#define __WK_HX71708_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "at32f425_wk_config.h"

/* exported types -------------------------------------------------------------*/
typedef enum
{
  HX71708_CH_LL = 0,   /* PD_SCK_LL_PIN / DOUT_LL_PIN (left load cell) */
  HX71708_CH_RR = 1,   /* PD_SCK_RR_PIN / DOUT_RR_PIN (right load cell) */
  HX71708_CH_NUM
} hx71708_channel_type;

/* exported functions ------------------------------------------------------- */

  /* configure gpio/exint for both hx71708 channels and run the power-up
     init pulse each chip needs before its first conversion */
  void wk_hx71708_init(void);

  /* call from the DOUT falling-edge exint isr for this channel: masks
     the line and sets a ready hint for wk_hx71708_get_sample() to
     service. Does not touch the sensor itself - kept minimal so a
     noisy/oscillating line can't turn into an isr storm. */
  void wk_hx71708_dout_isr(hx71708_channel_type ch);

  /* call every main loop iteration: services ch's ready hint (if any) -
     clocks out the conversion (~50us blocking) and re-arms the exint
     line. 1 and fills *raw_value on an accepted sample, 0 otherwise
     (nothing pending, or dropped as arriving too soon to be real) */
  uint8_t wk_hx71708_get_sample(hx71708_channel_type ch, int32_t *raw_value);

  /* call periodically (e.g. once per main loop): if ch hasn't produced a
     sample within the datasheet-derived timeout, force-resets it and
     returns 1; returns 0 otherwise */
  uint8_t wk_hx71708_check_stale(hx71708_channel_type ch);

#ifdef __cplusplus
}
#endif

#endif
