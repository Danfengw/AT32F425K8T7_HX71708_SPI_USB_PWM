/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __WK_TASK_H
#define __WK_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "at32f425_wk_config.h"
#include "wk_hx71708.h"

/* exported types -------------------------------------------------------------*/
typedef enum
{
  APP_STATE_CALIBRATION = 0,
  APP_STATE_NORMAL      = 1,
  APP_STATE_ERROR        = 2
} app_state_type;

/* exported constants --------------------------------------------------------*/
/* protocol status byte values, shared with the usb command/response layer */
#define APP_STATUS_NORMAL       (uint8_t)(0x00)
#define APP_STATUS_CALIBRATING  (uint8_t)(0x01)
#define APP_STATUS_FAILED       (uint8_t)(0xFF)

/* exported functions ------------------------------------------------------- */

  /* reset the state machine to APP_STATE_CALIBRATION and clear accumulators */
  void wk_task_init(void);

  /* run one step of the current state's work; call every main loop iteration */
  void wk_task_dispatch(void);

  /* current state as the protocol's status byte (0x00/0x01/0xFF) */
  uint8_t wk_task_get_status_byte(void);

  /* calibration offset for ch (0 until calibration has completed for it) */
  int32_t wk_task_get_offset(hx71708_channel_type ch);

  /* tare-corrected reading for ch; returns 1 and fills *value only when the
     state machine is in APP_STATE_NORMAL, 0 (and *value = 0) otherwise */
  uint8_t wk_task_get_tared_reading(hx71708_channel_type ch, int32_t *value);

#ifdef __cplusplus
}
#endif

#endif
