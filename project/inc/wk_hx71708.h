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

  /* configure gpio pins used by both hx71708 channels and run the
     power-up init pulse each chip needs before its first conversion */
  void wk_hx71708_init(void);

  /* returns 1 when the channel has a conversion ready to read, 0 otherwise */
  uint8_t wk_hx71708_is_ready(hx71708_channel_type ch);

  /* blocking: waits (with timeout) until the channel is ready, then reads it */
  uint8_t wk_hx71708_read_raw(hx71708_channel_type ch, int32_t *raw_value);

#ifdef __cplusplus
}
#endif

#endif
