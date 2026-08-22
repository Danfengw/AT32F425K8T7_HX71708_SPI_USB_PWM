/**
  **************************************************************************
  * @file     st7789_app.c
  * @brief    st7789 display application layer
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "st7789_app.h"
#include "wk_st7789.h"

/* RGB565 */
#define ST7789_APP_COLOR_GREEN    (uint16_t)(0x07E0)

/**
  * @brief  bring up the panel and paint a solid-color test pattern.
  * @param  none
  * @retval none
  */
void st7789_app_init(void)
{
  if(wk_st7789_init() != 0)
  {
    return;
  }

  /* solid-color test pattern: confirms the spi/dma link and panel
     bring-up without depending on text/graphics rendering yet */
  wk_st7789_fill_screen(ST7789_APP_COLOR_GREEN);
}
