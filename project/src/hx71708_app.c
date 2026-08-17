/**
  **************************************************************************
  * @file     hx71708_app.c
  * @brief    hx71708 load-cell application layer: owns the usb-cdc
  *           command/response protocol on top of wk_task's calibration/
  *           normal/error state machine.
  *
  * host -> device command frame: [header][cmd id][ctrl][cr][lf]
  * device -> host response frame: [header][device id][status][ll raw
  *           24-bit][rr raw 24-bit][cr][lf]
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hx71708_app.h"
#include "wk_hx71708.h"
#include "wk_task.h"
#include "usb_app.h"
#include <string.h>

/* host -> device command frame */
#define HX71708_CMD_HEADER0       (uint8_t)(0x3A)
#define HX71708_CMD_HEADER1       (uint8_t)(0x1A)
#define HX71708_CMD_FRAME_SIZE    (5)
#define HX71708_CMD_CTRL_READ     (uint8_t)(0x00)  /* request a reading */
#define HX71708_CMD_CTRL_OFFSET   (uint8_t)(0x01)  /* request calibration offsets */

/* device -> host response frame */
#define HX71708_RSP_HEADER0       (uint8_t)(0x3A)
#define HX71708_RSP_HEADER1       (uint8_t)(0x2A)
#define HX71708_RSP_FRAME_SIZE    (11)

#define HX71708_RX_ACC_SIZE       (32)

/**
  * @brief  build and send the response frame for one parsed command.
  * @param  ctrl: command byte from the host request (read vs offset query)
  * @retval none
  */
static void hx71708_handle_command(uint8_t ctrl)
{
  uint8_t resp[HX71708_RSP_FRAME_SIZE];
  uint32_t v1, v2;
  int32_t t1, t2;

  if(ctrl == HX71708_CMD_CTRL_READ)
  {
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
  }
  else if(ctrl == HX71708_CMD_CTRL_OFFSET)
  {
    /* offset-response marker; payload only valid once calibration is done */
    resp[2] = APP_STATUS_CALIBRATING;

    if(wk_task_get_status_byte() == APP_STATUS_NORMAL)
    {
      v1 = (uint32_t)wk_task_get_offset(HX71708_CH_LL) & 0x00FFFFFFUL;
      v2 = (uint32_t)wk_task_get_offset(HX71708_CH_RR) & 0x00FFFFFFUL;
    }
    else
    {
      v1 = 0;
      v2 = 0;
    }
  }
  else
  {
    /* unknown command: ignore silently */
    return;
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
  * @brief  drain any new usb-cdc rx bytes, resync/parse 5-byte command
  *         frames out of them, and answer each one immediately.
  * @param  none
  * @retval none
  */
static void hx71708_process_usb_commands(void)
{
  static uint8_t rx_acc[HX71708_RX_ACC_SIZE];
  static uint16_t rx_acc_len = 0;
  uint8_t chunk[64];
  uint16_t n;

  n = wk_usb_cdc_recv(chunk);
  if(n > 0)
  {
    if((uint16_t)(rx_acc_len + n) > sizeof(rx_acc))
    {
      /* overflow: drop what we have and resync on whatever arrives next */
      rx_acc_len = 0;
    }
    else
    {
      memmove(&rx_acc[rx_acc_len], chunk, n);
      rx_acc_len = (uint16_t)(rx_acc_len + n);
    }
  }

  while(rx_acc_len >= HX71708_CMD_FRAME_SIZE)
  {
    uint16_t idx;
    uint8_t found = 0;

    for(idx = 0; (uint16_t)(idx + 1) < rx_acc_len; idx++)
    {
      if(rx_acc[idx] == HX71708_CMD_HEADER0 && rx_acc[idx + 1] == HX71708_CMD_HEADER1)
      {
        found = 1;
        break;
      }
    }

    if(!found)
    {
      /* keep the last byte in case a header started here */
      rx_acc[0] = rx_acc[rx_acc_len - 1];
      rx_acc_len = 1;
      break;
    }

    if(idx > 0)
    {
      memmove(rx_acc, &rx_acc[idx], (size_t)(rx_acc_len - idx));
      rx_acc_len = (uint16_t)(rx_acc_len - idx);
    }

    if(rx_acc_len < HX71708_CMD_FRAME_SIZE)
    {
      break;
    }

    if(rx_acc[3] != 0x0D || rx_acc[4] != 0x0A)
    {
      /* header bytes matched inside noise; resync by one byte */
      memmove(rx_acc, &rx_acc[1], (size_t)(rx_acc_len - 1));
      rx_acc_len--;
      continue;
    }

    hx71708_handle_command(rx_acc[2]);

    memmove(rx_acc, &rx_acc[HX71708_CMD_FRAME_SIZE], (size_t)(rx_acc_len - HX71708_CMD_FRAME_SIZE));
    rx_acc_len = (uint16_t)(rx_acc_len - HX71708_CMD_FRAME_SIZE);
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
  * @brief  run one step of the load-cell task dispatcher and answer any
  *         pending usb-cdc command frame.
  * @param  none
  * @retval none
  */
void hx71708_app_task(void)
{
  /* runs whichever state (calibration/normal/error) is currently active */
  wk_task_dispatch();

  /* purely request/response: only ever transmits in reply to a host command */
  hx71708_process_usb_commands();
}
