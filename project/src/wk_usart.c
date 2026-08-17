/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_usart.c
  * @brief    work bench config program
  **************************************************************************
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */
/* add user code end Header */

/* Includes ------------------------------------------------------------------*/
#include "wk_usart.h"
#include <string.h>

/* add user code begin 0 */

#define USART1_DMA_TX_CHANNEL     DMA1_CHANNEL2
#define USART1_DMA_TX_FLAG        DMA1_FDT2_FLAG
#define USART1_DMA_RX_CHANNEL     DMA1_CHANNEL3
#define USART1_RX_BUFFER_SIZE     (64)

static volatile uint8_t usart1_dma_tx_busy = 0;

static uint8_t usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
static volatile uint8_t usart1_rx_frame_ready = 0;
static volatile uint16_t usart1_rx_frame_len = 0;

/**
  * @brief  (re)arm the rx dma channel to fill usart1_rx_buffer from the
  *         start. safe to call whenever the channel is disabled (init,
  *         or after the idle isr has pulled a frame out).
  * @param  none
  * @retval none
  */
static void usart1_dma_rx_rearm(void)
{
  dma_channel_enable(USART1_DMA_RX_CHANNEL, FALSE);
  USART1_DMA_RX_CHANNEL->maddr = (uint32_t)usart1_rx_buffer;
  USART1_DMA_RX_CHANNEL->dtcnt = USART1_RX_BUFFER_SIZE;
  dma_channel_enable(USART1_DMA_RX_CHANNEL, TRUE);
}

/**
  * @brief  configure dma1_channel2 (usart1 tx) and dma1_channel3
  *         (usart1 rx, continuously refilled, framed by usart idle).
  * @param  none
  * @retval none
  */
static void wk_usart1_dma_config(void)
{
  dma_init_type dma_init_struct;

  /* channel2: usart1 tx. memory address/length armed per-transfer by
     wk_usart1_dma_send(); only the fixed peripheral-side settings here. */
  dma_reset(USART1_DMA_TX_CHANNEL);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
  dma_init_struct.peripheral_base_addr = (uint32_t)&(USART1->dt);
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
  dma_init_struct.memory_inc_enable = TRUE;
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
  dma_init_struct.loop_mode_enable = FALSE;
  dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
  dma_init_struct.buffer_size = 0;
  dma_init_struct.memory_base_addr = 0;
  dma_init(USART1_DMA_TX_CHANNEL, &dma_init_struct);
  dma_flexible_config(DMA1, FLEX_CHANNEL2, DMA_FLEXIBLE_UART1_TX);
  dma_interrupt_enable(USART1_DMA_TX_CHANNEL, DMA_FDT_INT, TRUE);

  /* channel3: usart1 rx. runs continuously; frame boundaries are found via
     the usart idle-line interrupt, not a dma completion interrupt. */
  dma_reset(USART1_DMA_RX_CHANNEL);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
  dma_init_struct.peripheral_base_addr = (uint32_t)&(USART1->dt);
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
  dma_init_struct.memory_inc_enable = TRUE;
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
  dma_init_struct.loop_mode_enable = FALSE;
  dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
  dma_init_struct.buffer_size = USART1_RX_BUFFER_SIZE;
  dma_init_struct.memory_base_addr = (uint32_t)usart1_rx_buffer;
  dma_init(USART1_DMA_RX_CHANNEL, &dma_init_struct);
  dma_flexible_config(DMA1, FLEX_CHANNEL3, DMA_FLEXIBLE_UART1_RX);

  dma_channel_enable(USART1_DMA_RX_CHANNEL, TRUE);
}

/* add user code end 0 */

/**
  * @brief  init usart1 function
  * @param  none
  * @retval none
  */
void wk_usart1_init(void)
{
  /* add user code begin usart1_init 0 */

  /* add user code end usart1_init 0 */

  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin usart1_init 1 */

  /* add user code end usart1_init 1 */

  /* configure the TX pin */
  gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE9, GPIO_MUX_1);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_9;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);

  /* configure the RX pin */
  gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE10, GPIO_MUX_1);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_10;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);

  /* configure param */
  usart_init(USART1, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
  usart_transmitter_enable(USART1, TRUE);
  usart_receiver_enable(USART1, TRUE);
  usart_parity_selection_config(USART1, USART_PARITY_NONE);

  usart_hardware_flow_control_set(USART1, USART_HARDWARE_FLOW_NONE);

  /* add user code begin usart1_init 2 */

  wk_usart1_dma_config();
  usart_dma_transmitter_enable(USART1, TRUE);
  usart_dma_receiver_enable(USART1, TRUE);
  usart_interrupt_enable(USART1, USART_IDLE_INT, TRUE);

  /* add user code end usart1_init 2 */

  usart_enable(USART1, TRUE);

  /* add user code begin usart1_init 3 */

  /* add user code end usart1_init 3 */
}

/**
  * @brief  start a dma-driven tx transfer. does nothing if a transfer is
  *         already in flight (check wk_usart1_dma_tx_busy() first).
  * @param  data: buffer to send, must stay valid until the transfer
  *         completes (wk_usart1_dma_tx_busy() returns 0)
  * @param  len: number of bytes to send
  * @retval none
  */
void wk_usart1_dma_send(uint8_t *data, uint16_t len)
{
  if(usart1_dma_tx_busy || len == 0)
  {
    return;
  }

  usart1_dma_tx_busy = 1;

  dma_channel_enable(USART1_DMA_TX_CHANNEL, FALSE);
  dma_flag_clear(USART1_DMA_TX_FLAG);
  USART1_DMA_TX_CHANNEL->maddr = (uint32_t)data;
  USART1_DMA_TX_CHANNEL->dtcnt = len;
  dma_channel_enable(USART1_DMA_TX_CHANNEL, TRUE);
}

/**
  * @brief  whether a dma tx transfer is currently in flight.
  * @param  none
  * @retval 1 if busy, 0 if idle
  */
uint8_t wk_usart1_dma_tx_busy(void)
{
  return usart1_dma_tx_busy;
}

/**
  * @brief  if a rx frame is ready, copy it out and re-arm dma for the next
  *         one. call this regularly from the main loop.
  * @param  dst: destination buffer
  * @param  max_len: size of dst
  * @retval number of bytes copied (0 if no frame is ready yet)
  */
uint16_t wk_usart1_dma_rx_get_frame(uint8_t *dst, uint16_t max_len)
{
  uint16_t len;

  if(!usart1_rx_frame_ready)
  {
    return 0;
  }

  len = usart1_rx_frame_len;
  if(len > max_len)
  {
    len = max_len;
  }
  memcpy(dst, usart1_rx_buffer, len);

  usart1_rx_frame_ready = 0;
  usart1_dma_rx_rearm();

  return len;
}

/**
  * @brief  dma1_channel2 (usart1 tx) completion notification.
  * @param  none
  * @retval none
  */
void wk_usart1_dma_tx_irq_handler(void)
{
  if(dma_interrupt_flag_get(USART1_DMA_TX_FLAG))
  {
    dma_flag_clear(USART1_DMA_TX_FLAG);
    dma_channel_enable(USART1_DMA_TX_CHANNEL, FALSE);
    usart1_dma_tx_busy = 0;
  }
}

/**
  * @brief  usart1 idle-line notification: the line just went quiet after
  *         receiving one or more bytes, so whatever dma has copied into
  *         usart1_rx_buffer so far is a complete frame. the dma channel is
  *         left disabled (frame data held safely) until
  *         wk_usart1_dma_rx_get_frame() re-arms it.
  * @param  none
  * @retval none
  */
void wk_usart1_irq_handler(void)
{
  if(usart_interrupt_flag_get(USART1, USART_IDLEF_FLAG))
  {
    /* reading dt is what clears the idle flag */
    usart_data_receive(USART1);

    dma_channel_enable(USART1_DMA_RX_CHANNEL, FALSE);
    usart1_rx_frame_len = USART1_RX_BUFFER_SIZE - dma_data_number_get(USART1_DMA_RX_CHANNEL);
    usart1_rx_frame_ready = 1;
  }
}

/* add user code begin 1 */

/* add user code end 1 */
