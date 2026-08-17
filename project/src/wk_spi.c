/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_spi.c
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
#include "wk_spi.h"

/* add user code begin 0 */

#define SPI1_DMA_TX_CHANNEL     DMA1_CHANNEL1
#define SPI1_DMA_TX_FLAG        DMA1_FDT1_FLAG

static volatile uint8_t spi1_dma_tx_busy = 0;

/**
  * @brief  configure dma1_channel1 as the spi1 half-duplex tx channel.
  *         memory address/length are armed per-transfer by wk_spi1_dma_send();
  *         only the fixed peripheral-side settings are configured here.
  * @param  none
  * @retval none
  */
static void wk_spi1_dma_config(void)
{
  dma_init_type dma_init_struct;

  dma_reset(SPI1_DMA_TX_CHANNEL);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
  dma_init_struct.peripheral_base_addr = (uint32_t)&(SPI1->dt);
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
  dma_init_struct.memory_inc_enable = TRUE;
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
  dma_init_struct.loop_mode_enable = FALSE;
  dma_init_struct.priority = DMA_PRIORITY_HIGH;
  dma_init_struct.buffer_size = 0;
  dma_init_struct.memory_base_addr = 0;
  dma_init(SPI1_DMA_TX_CHANNEL, &dma_init_struct);
  dma_flexible_config(DMA1, FLEX_CHANNEL1, DMA_FLEXIBLE_SPI1_TX);

  dma_interrupt_enable(SPI1_DMA_TX_CHANNEL, DMA_FDT_INT, TRUE);
}

/* add user code end 0 */

/**
  * @brief  init spi1 function
  * @param  none
  * @retval none
  */
void wk_spi1_init(void)
{
  /* add user code begin spi1_init 0 */

  /* add user code end spi1_init 0 */

  gpio_init_type gpio_init_struct;
  spi_init_type spi_init_struct;

  gpio_default_para_init(&gpio_init_struct);
  spi_default_para_init(&spi_init_struct);

  /* add user code begin spi1_init 1 */

  /* add user code end spi1_init 1 */

  /* configure the SCK pin */
  gpio_pin_mux_config(ST7789_SCK_GPIO_PORT, GPIO_PINS_SOURCE5, GPIO_MUX_0);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = ST7789_SCK_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(ST7789_SCK_GPIO_PORT, &gpio_init_struct);

  /* configure the MOSI pin */
  gpio_pin_mux_config(ST7789_MOSI_GPIO_PORT, GPIO_PINS_SOURCE7, GPIO_MUX_0);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = ST7789_MOSI_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(ST7789_MOSI_GPIO_PORT, &gpio_init_struct);

  /* configure param */
  /* Set transmission direction using spi_half_duplex_direction_set(spi_type* spi_x, spi_half_duplex_direction_type direction) */
  spi_init_struct.transmission_mode = SPI_TRANSMIT_HALF_DUPLEX_TX;
  spi_init_struct.master_slave_mode = SPI_MODE_MASTER;
  spi_init_struct.frame_bit_num = SPI_FRAME_8BIT;
  spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
  spi_init_struct.mclk_freq_division = SPI_MCLK_DIV_3;
  spi_init_struct.clock_polarity = SPI_CLOCK_POLARITY_LOW;
  spi_init_struct.clock_phase = SPI_CLOCK_PHASE_1EDGE;
  spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
  spi_init(SPI1, &spi_init_struct);

  /* add user code begin spi1_init 2 */

  wk_spi1_dma_config();
  spi_i2s_dma_transmitter_enable(SPI1, TRUE);

  /* add user code end spi1_init 2 */

  spi_enable(SPI1, TRUE);

  /* add user code begin spi1_init 3 */

  /* add user code end spi1_init 3 */
}

/**
  * @brief  start a dma-driven half-duplex tx transfer. does nothing if a
  *         transfer is already in flight (check wk_spi1_dma_busy() first).
  * @param  data: buffer to send, must stay valid until the transfer
  *         completes (wk_spi1_dma_busy() returns 0)
  * @param  len: number of bytes to send
  * @retval none
  */
void wk_spi1_dma_send(uint8_t *data, uint16_t len)
{
  if(spi1_dma_tx_busy || len == 0)
  {
    return;
  }

  spi1_dma_tx_busy = 1;

  dma_channel_enable(SPI1_DMA_TX_CHANNEL, FALSE);
  dma_flag_clear(SPI1_DMA_TX_FLAG);
  SPI1_DMA_TX_CHANNEL->maddr = (uint32_t)data;
  SPI1_DMA_TX_CHANNEL->dtcnt = len;
  dma_channel_enable(SPI1_DMA_TX_CHANNEL, TRUE);
}

/**
  * @brief  whether a dma tx transfer is currently in flight.
  * @param  none
  * @retval 1 if busy, 0 if idle
  */
uint8_t wk_spi1_dma_busy(void)
{
  return spi1_dma_tx_busy;
}

/**
  * @brief  dma1_channel1 (spi1 tx) completion notification.
  * @param  none
  * @retval none
  */
void wk_spi1_dma_irq_handler(void)
{
  if(dma_interrupt_flag_get(SPI1_DMA_TX_FLAG))
  {
    dma_flag_clear(SPI1_DMA_TX_FLAG);
    dma_channel_enable(SPI1_DMA_TX_CHANNEL, FALSE);
    spi1_dma_tx_busy = 0;
  }
}

/* add user code begin 1 */

/* add user code end 1 */
