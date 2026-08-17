/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __HX71708_APP_H
#define __HX71708_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "at32f425_wk_config.h"

/* exported functions ------------------------------------------------------- */

  /* bring up both hx71708 channels and start the calibration/normal/error
     task dispatcher. call once at startup, after wk_usb_app_init(). */
  void hx71708_app_init(void);

  /* run one step of the load-cell task dispatcher and answer any pending
     usb-cdc command frame. call every main loop iteration. */
  void hx71708_app_task(void);

#ifdef __cplusplus
}
#endif

#endif
