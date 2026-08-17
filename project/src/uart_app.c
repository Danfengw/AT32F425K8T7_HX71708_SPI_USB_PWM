/**
  **************************************************************************
  * @file     uart_app.c
  * @brief    controller comm protocol codec ("涛涛控制器通讯协议V1.1.29")
  *
  * Standalone frame encode/decode + crc16. Not wired to any uart driver or
  * business logic yet - uart_app_parse_feed() takes raw bytes from
  * wherever they end up coming from (e.g. wk_usart1_dma_rx_get_frame()),
  * and uart_app_encode_frame() produces bytes ready for whatever sends
  * them (e.g. wk_usart1_dma_send()). That wiring, and any per-function-code
  * handling, is a later step.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "uart_app.h"
#include <string.h>

#define UART_APP_RX_ACC_SIZE      (2 * UART_APP_MAX_FRAME_LEN)

static uint8_t rx_acc[UART_APP_RX_ACC_SIZE];
static uint16_t rx_acc_len = 0;

static uart_app_frame_type ready_frame;
static volatile uint8_t frame_ready = 0;

/**
  * @brief  crc16 over data[0..len-1]. reference algorithm from the
  *         protocol spec, ported bit-for-bit (only the length counter is
  *         widened from uint8_t to uint16_t; the algorithm itself is
  *         unchanged).
  * @param  data: buffer to checksum
  * @param  len: number of bytes
  * @retval crc16 value
  */
uint16_t uart_app_crc16(const uint8_t *data, uint16_t len)
{
  uint16_t crc16 = 0;
  uint16_t i;

  for(i = 0; i < len; i++)
  {
    crc16 = (uint16_t)((crc16 >> 8) | (crc16 << 8));
    crc16 ^= data[i];
    crc16 ^= (uint16_t)((crc16 & 0xFF) >> 4);
    crc16 ^= (uint16_t)((crc16 << 8) << 4);
    crc16 ^= (uint16_t)(((crc16 & 0xFF) << 4) << 1);
  }

  return crc16;
}

/**
  * @brief  pack role/permission/device-id into a Data[1] address byte.
  * @param  is_slave: 0 = master role, nonzero = slave role
  * @param  permission: read/write permission (Data[1] bit4-6)
  * @param  device_id: slave device id, e.g. UART_APP_DEV_CONTROLLER (Data[1] bit0-3)
  * @retval the packed address byte
  */
uint8_t uart_app_build_address(uint8_t is_slave, uart_app_perm_type permission, uint8_t device_id)
{
  uint8_t addr = 0;

  if(is_slave)
  {
    addr |= UART_APP_ADDR_ROLE_SLAVE;
  }

  addr |= (uint8_t)(((uint8_t)permission & 0x07) << 4);
  addr |= (uint8_t)(device_id & 0x0F);

  return addr;
}

/**
  * @brief  encode a complete frame (header..crc16) into out_buf.
  * @param  out_buf: destination, must be at least UART_APP_MAX_FRAME_LEN bytes
  * @param  address: Data[1] address/permission byte (see uart_app_build_address())
  * @param  func_code: function code (protocol section 2)
  * @param  data: payload bytes, may be NULL if data_len is 0
  * @param  data_len: payload length
  * @retval total frame length in bytes, or 0 if data_len is out of range
  */
uint16_t uart_app_encode_frame(uint8_t *out_buf, uint8_t address, uint16_t func_code,
                                const uint8_t *data, uint8_t data_len)
{
  uint16_t crc;

  if(data_len > UART_APP_MAX_DATA_LEN)
  {
    return 0;
  }

  out_buf[0] = UART_APP_FRAME_HEADER;
  out_buf[1] = address;
  out_buf[2] = (uint8_t)(func_code & 0xFF);
  out_buf[3] = (uint8_t)((func_code >> 8) & 0xFF);
  out_buf[4] = data_len;

  if(data_len > 0)
  {
    memcpy(&out_buf[UART_APP_PREFIX_LEN], data, data_len);
  }

  crc = uart_app_crc16(out_buf, (uint16_t)(UART_APP_PREFIX_LEN + data_len));
  out_buf[UART_APP_PREFIX_LEN + data_len]     = (uint8_t)(crc >> 8);
  out_buf[UART_APP_PREFIX_LEN + data_len + 1] = (uint8_t)(crc & 0xFF);

  return (uint16_t)(UART_APP_PREFIX_LEN + data_len + 2);
}

/**
  * @brief  feed newly-arrived raw bytes into the parser. resyncs on the
  *         0x5A header and validates crc16 before exposing a frame;
  *         malformed/corrupt bytes in between are silently discarded.
  *         if a frame is already waiting (uart_app_frame_ready()), newly
  *         completed frames are dropped rather than overwriting it - this
  *         is a single-slot design matching the protocol's one-question-
  *         one-answer usage.
  * @param  chunk: newly-arrived bytes
  * @param  len: number of bytes in chunk
  * @retval none
  */
void uart_app_parse_feed(const uint8_t *chunk, uint16_t len)
{
  uint16_t i;

  for(i = 0; i < len; i++)
  {
    if(rx_acc_len >= UART_APP_RX_ACC_SIZE)
    {
      /* overflow: drop everything and resync on whatever arrives next */
      rx_acc_len = 0;
    }
    rx_acc[rx_acc_len++] = chunk[i];
  }

  for(;;)
  {
    uint16_t idx;
    uint8_t found = 0;
    uint16_t data_len;
    uint16_t frame_len;

    for(idx = 0; idx < rx_acc_len; idx++)
    {
      if(rx_acc[idx] == UART_APP_FRAME_HEADER)
      {
        found = 1;
        break;
      }
    }

    if(!found)
    {
      /* no header anywhere in what we have: nothing usable to keep */
      rx_acc_len = 0;
      break;
    }

    if(idx > 0)
    {
      memmove(rx_acc, &rx_acc[idx], (size_t)(rx_acc_len - idx));
      rx_acc_len = (uint16_t)(rx_acc_len - idx);
    }

    /* need the 5-byte prefix (header/addr/funclo/funchi/len) to know the
       full frame length */
    if(rx_acc_len < UART_APP_PREFIX_LEN)
    {
      break;
    }

    data_len = rx_acc[4];
    if(data_len > UART_APP_MAX_DATA_LEN)
    {
      /* not a real frame: drop the header byte and keep resyncing */
      memmove(rx_acc, &rx_acc[1], (size_t)(rx_acc_len - 1));
      rx_acc_len--;
      continue;
    }

    frame_len = (uint16_t)(UART_APP_PREFIX_LEN + data_len + 2);

    if(rx_acc_len < frame_len)
    {
      /* whole frame hasn't arrived yet */
      break;
    }

    {
      uint16_t crc_calc = uart_app_crc16(rx_acc, (uint16_t)(UART_APP_PREFIX_LEN + data_len));
      uint16_t crc_recv = ((uint16_t)rx_acc[UART_APP_PREFIX_LEN + data_len] << 8)
                            | rx_acc[UART_APP_PREFIX_LEN + data_len + 1];

      if(crc_calc == crc_recv)
      {
        if(!frame_ready)
        {
          ready_frame.address = rx_acc[1];
          ready_frame.func_code = (uint16_t)rx_acc[2] | ((uint16_t)rx_acc[3] << 8);
          ready_frame.data_len = (uint8_t)data_len;
          memcpy(ready_frame.data, &rx_acc[UART_APP_PREFIX_LEN], data_len);
          frame_ready = 1;
        }

        memmove(rx_acc, &rx_acc[frame_len], (size_t)(rx_acc_len - frame_len));
        rx_acc_len = (uint16_t)(rx_acc_len - frame_len);
      }
      else
      {
        /* bad crc: drop the header byte and resync */
        memmove(rx_acc, &rx_acc[1], (size_t)(rx_acc_len - 1));
        rx_acc_len--;
      }
    }
  }
}

/**
  * @brief  whether a validated frame is waiting to be consumed.
  * @param  none
  * @retval 1 if a frame is ready, 0 otherwise
  */
uint8_t uart_app_frame_ready(void)
{
  return frame_ready;
}

/**
  * @brief  copy the ready frame out and clear the ready flag.
  * @param  out_frame: destination
  * @retval 1 if a frame was copied, 0 if none was ready
  */
uint8_t uart_app_get_frame(uart_app_frame_type *out_frame)
{
  if(!frame_ready)
  {
    return 0;
  }

  *out_frame = ready_frame;
  frame_ready = 0;
  return 1;
}
