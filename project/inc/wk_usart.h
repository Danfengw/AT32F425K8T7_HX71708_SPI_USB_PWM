/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_usart.h
  * @brief    header file of work bench config
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

/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __WK_USART_H
#define __WK_USART_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "at32f425_wk_config.h"

/* private includes -------------------------------------------------------------*/
/* add user code begin private includes */

/* add user code end private includes */

/* exported types -------------------------------------------------------------*/
/* add user code begin exported types */

/* add user code end exported types */

/* exported constants --------------------------------------------------------*/
/* add user code begin exported constants */

/* add user code end exported constants */

/* exported macro ------------------------------------------------------------*/
/* add user code begin exported macro */

/* add user code end exported macro */

/* exported functions ------------------------------------------------------- */

  /* init usart1 function. */
  void wk_usart1_init(void);

  /* start a dma-driven tx transfer (data must stay valid until
     wk_usart1_dma_tx_busy() returns 0). does nothing if a transfer is
     already in flight. */
  void wk_usart1_dma_send(uint8_t *data, uint16_t len);

  /* 1 while a dma tx transfer is in flight, 0 once complete */
  uint8_t wk_usart1_dma_tx_busy(void);

  /* if a rx frame is ready (usart went idle after receiving one), copy up
     to max_len bytes of it into dst and re-arm dma for the next frame.
     returns the number of bytes copied (0 if no frame is ready yet). */
  uint16_t wk_usart1_dma_rx_get_frame(uint8_t *dst, uint16_t max_len);

  /* dma1_channel2 (usart1 tx) completion notification, called from
     DMA1_Channel3_2_IRQHandler in at32f425_int.c */
  void wk_usart1_dma_tx_irq_handler(void);

  /* usart1 idle-line notification, called from USART1_IRQHandler in
     at32f425_int.c */
  void wk_usart1_irq_handler(void);

/* add user code begin exported functions */

/* add user code end exported functions */

#ifdef __cplusplus
}
#endif

#endif
