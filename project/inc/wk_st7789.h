/**
  **************************************************************************
  * @file     wk_st7789.h
  * @brief    hardware port layer binding driver_st7789 (libraries/st7789)
  *           to spi1+dma and the board's reset/rs/cs/backlight gpio pins
  **************************************************************************
  */
#ifndef __WK_ST7789_H
#define __WK_ST7789_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f425_wk_config.h"
#include "driver_st7789.h"

/* panel is a 135x240 physical ST7789 module, run in landscape */
#define WK_ST7789_WIDTH     (240)
#define WK_ST7789_HEIGHT    (135)

  /* reset + init sequence, then paints the screen black and turns the
     display and backlight on. spi1 (wk_spi1_init()) must already be
     initialized. call once from main().
     @retval 0 on success, driver_st7789's st7789_init() error code otherwise */
  uint8_t wk_st7789_init(void);

  /* fill a rectangle (inclusive coordinates, 0-indexed against
     WK_ST7789_WIDTH/HEIGHT) with an RGB565 color. out-of-range or
     inverted rectangles are silently ignored. */
  void wk_st7789_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color565);

  /* fill the entire visible screen with an RGB565 color */
  void wk_st7789_fill_screen(uint16_t color565);

#ifdef __cplusplus
}
#endif

#endif
