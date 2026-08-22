/**
  **************************************************************************
  * @file     hx71708_app.c
  * @brief    hx71708 load-cell application layer: owns the usb-cdc
  *           broadcast on top of wk_task's calibration/normal/error state
  *           machine.
  *
  * device -> host broadcast frame (unsolicited, sent every
  * HX71708_BROADCAST_INTERVAL_US regardless of whether the host asks):
  *   [header][device id][status][ll raw 24-bit][rr raw 24-bit][cr][lf]
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hx71708_app.h"
#include "wk_hx71708.h"
#include "wk_task.h"
#include "wk_system.h"
#include "usb_app.h"

/* device -> host broadcast frame */
#define HX71708_RSP_HEADER0       (uint8_t)(0x3A)
#define HX71708_RSP_HEADER1       (uint8_t)(0x2A)
#define HX71708_RSP_FRAME_SIZE    (11)

/* how often the current reading is pushed out the usb-cdc port,
   unsolicited; matches the hx71708's own 10hz conversion rate */
#define HX71708_BROADCAST_INTERVAL_US    (uint32_t)(100000)

/**
  * @brief  build the broadcast frame for the current reading and send it
  *         out the usb-cdc port (best-effort: dropped if the host isn't
  *         keeping up, since the next broadcast supersedes it anyway).
  * @param  none
  * @retval none
  */
static void hx71708_broadcast(void)
{
  uint8_t resp[HX71708_RSP_FRAME_SIZE];
  uint32_t v1, v2;
  int32_t t1, t2;

  resp[2] = wk_task_get_status_byte();

  if(wk_task_get_tared_reading(HX71708_CH_LL, &t1) && wk_task_get_tared_reading(HX71708_CH_RR, &t2))
  {
    v1 = (uint32_t)t1 & 0x00FFFFFFUL;
    v2 = (uint32_t)t2 & 0x00FFFFFFUL;
  }
  else
  {
    v1 = 0;
    v2 = 0;
  }

  resp[0] = HX71708_RSP_HEADER0;
  resp[1] = HX71708_RSP_HEADER1;
  resp[3] = (uint8_t)(v1 >> 16);
  resp[4] = (uint8_t)(v1 >> 8);
  resp[5] = (uint8_t)(v1);
  resp[6] = (uint8_t)(v2 >> 16);
  resp[7] = (uint8_t)(v2 >> 8);
  resp[8] = (uint8_t)(v2);
  resp[9] = 0x0D;
  resp[10] = 0x0A;

  wk_usb_cdc_send(resp, HX71708_RSP_FRAME_SIZE);
}

/**
  * @brief  broadcast the current reading once every
  *         HX71708_BROADCAST_INTERVAL_US, unprompted.
  * @param  none
  * @retval none
  */
static void hx71708_broadcast_task(void)
{
  static uint32_t broadcast_ticks;

  if(wk_tick_elapsed_us(&broadcast_ticks, HX71708_BROADCAST_INTERVAL_US))
  {
    hx71708_broadcast();
  }
}

/**
  * @brief  bring up both hx71708 channels and start the calibration/
  *         normal/error task dispatcher.
  * @param  none
  * @retval none
  */
void hx71708_app_init(void)
{
  wk_hx71708_init();
  wk_task_init();
}

/**
  * @brief  run one step of the load-cell task dispatcher and, once per
  *         broadcast period, push the current reading out unsolicited.
  * @param  none
  * @retval none
  */
void hx71708_app_task(void)
{
  /* runs whichever state (calibration/normal/error) is currently active */
  wk_task_dispatch();

  hx71708_broadcast_task();
}
