/**
  **************************************************************************
  * @file     st7789_app.h
  * @brief    st7789 display application layer
  **************************************************************************
  */
#ifndef __ST7789_APP_H
#define __ST7789_APP_H

#ifdef __cplusplus
extern "C" {
#endif

  /* bring up the st7789 panel (backlight + spi/dma-driven init) and paint
     a solid-color test pattern to confirm the link is alive. spi1 must
     already be initialized (wk_spi1_init()). call once from main(). */
  void st7789_app_init(void);

#ifdef __cplusplus
}
#endif

#endif
