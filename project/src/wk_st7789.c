/**
  **************************************************************************
  * @file     wk_st7789.c
  * @brief    hardware port layer binding driver_st7789 (libraries/st7789)
  *           to spi1+dma and the board's reset/rs/cs/backlight gpio pins
  *
  * driver_st7789's spi_write_cmd callback is synchronous (returns a status
  * code once the write is done), so this port starts a dma tx via
  * wk_spi1_dma_send() and blocks on wk_spi1_dma_busy() until it completes -
  * the actual byte clocking is offloaded to dma hardware, the driver just
  * doesn't have an async completion callback to hand the wait off to.
  *
  * The driver has no CS callback (it assumes hardware CS or a raw 3-wire
  * link), so CS is toggled here, wrapped around each dma transfer.
  *
  * driver_st7789's high-level helpers (st7789_draw_point/fill_rect/
  * write_string/clear) address ram using raw 0-319 coordinates with no
  * offset support, but this panel's visible 135x240 window sits at a
  * fixed offset within the controller's full addressable area (a known
  * quirk of this panel family - see WK_ST7789_COL/ROW_OFFSET below). So
  * this port bypasses those helpers and drives the low-level
  * st7789_set_column_address/st7789_set_row_address/st7789_memory_write
  * primitives directly, adding the offset itself.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wk_st7789.h"
#include "wk_spi.h"
#include "wk_system.h"

/* colstart/rowstart offsets for this panel family (135x240 physical,
   run in landscape here) are the standard values documented across
   several open-source st7789 ports for this module - verify against the
   actual hardware on first bring-up (image shifted -> adjust these) */
#define WK_ST7789_COL_OFFSET    (40)
#define WK_ST7789_ROW_OFFSET    (53)

/* MX | MV: column address order reversed + row/column exchange, the
   standard "landscape" memory access control for this panel family.
   OR in ST7789_ORDER_COLOR_BGR here if red/blue come out swapped on the
   actual panel. */
#define WK_ST7789_MADCTL_VALUE \
  (uint8_t)(ST7789_ORDER_COLUMN_RIGHT_TO_LEFT | ST7789_ORDER_PAGE_COLUMN_REVERSE)

/* pixels per dma chunk when filling a rectangle; keeps the stack buffer
   small while still amortizing per-transfer overhead */
#define WK_ST7789_FILL_CHUNK_PIXELS    (64)

static st7789_handle_t st7789_handle;

/**
  * @brief  spi1 is already brought up by wk_spi1_init() in main() before
  *         wk_st7789_init() runs, so this is a no-op.
  */
static uint8_t st7789_if_spi_init(void)
{
  return 0;
}

static uint8_t st7789_if_spi_deinit(void)
{
  return 0;
}

/**
  * @brief  synchronous spi write: assert cs, hand the buffer to the dma
  *         tx channel, block until it completes, deassert cs.
  */
static uint8_t st7789_if_spi_write_cmd(uint8_t *buf, uint16_t len)
{
  gpio_bits_reset(ST7789_CS_GPIO_PORT, ST7789_CS_PIN);

  wk_spi1_dma_send(buf, len);
  while(wk_spi1_dma_busy())
  {
  }

  gpio_bits_set(ST7789_CS_GPIO_PORT, ST7789_CS_PIN);

  return 0;
}

/**
  * @brief  rs/cs/reset pins are already configured as outputs by
  *         wk_gpio_config(), so init/deinit are no-ops here.
  */
static uint8_t st7789_if_cmd_data_gpio_init(void)
{
  return 0;
}

static uint8_t st7789_if_cmd_data_gpio_deinit(void)
{
  return 0;
}

static uint8_t st7789_if_cmd_data_gpio_write(uint8_t value)
{
  if(value)
  {
    gpio_bits_set(ST7789_RS_GPIO_PORT, ST7789_RS_PIN);
  }
  else
  {
    gpio_bits_reset(ST7789_RS_GPIO_PORT, ST7789_RS_PIN);
  }

  return 0;
}

static uint8_t st7789_if_reset_gpio_init(void)
{
  return 0;
}

static uint8_t st7789_if_reset_gpio_deinit(void)
{
  return 0;
}

static uint8_t st7789_if_reset_gpio_write(uint8_t value)
{
  if(value)
  {
    gpio_bits_set(ST7789_RESET_GPIO_PORT, ST7789_RESET_PIN);
  }
  else
  {
    gpio_bits_reset(ST7789_RESET_GPIO_PORT, ST7789_RESET_PIN);
  }

  return 0;
}

static void st7789_if_delay_ms(uint32_t ms)
{
  wk_delay_ms(ms);
}

/**
  * @brief  driver_st7789 requires a non-null debug_print; no debug/log
  *         channel is wired up for it yet, so this just discards output.
  */
static void st7789_if_debug_print(const char *const fmt, ...)
{
  (void)fmt;
}

/**
  * @brief  set the ram write window (with this panel's fixed offset
  *         applied) ready for a subsequent memory_write/continue_write.
  * @param  x0,y0,x1,y1: inclusive window, 0-indexed against
  *         WK_ST7789_WIDTH/HEIGHT
  * @retval none
  */
static void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  st7789_set_column_address(&st7789_handle, (uint16_t)(x0 + WK_ST7789_COL_OFFSET), (uint16_t)(x1 + WK_ST7789_COL_OFFSET));
  st7789_set_row_address(&st7789_handle, (uint16_t)(y0 + WK_ST7789_ROW_OFFSET), (uint16_t)(y1 + WK_ST7789_ROW_OFFSET));
}

/**
  * @brief  reset + init sequence, then paints the screen black and turns
  *         the display and backlight on.
  * @param  none
  * @retval 0 on success, st7789_init()'s error code otherwise
  */
uint8_t wk_st7789_init(void)
{
  uint8_t res;

  DRIVER_ST7789_LINK_INIT(&st7789_handle, st7789_handle_t);
  DRIVER_ST7789_LINK_SPI_INIT(&st7789_handle, st7789_if_spi_init);
  DRIVER_ST7789_LINK_SPI_DEINIT(&st7789_handle, st7789_if_spi_deinit);
  DRIVER_ST7789_LINK_SPI_WRITE_COMMAND(&st7789_handle, st7789_if_spi_write_cmd);
  DRIVER_ST7789_LINK_COMMAND_DATA_GPIO_INIT(&st7789_handle, st7789_if_cmd_data_gpio_init);
  DRIVER_ST7789_LINK_COMMAND_DATA_GPIO_DEINIT(&st7789_handle, st7789_if_cmd_data_gpio_deinit);
  DRIVER_ST7789_LINK_COMMAND_DATA_GPIO_WRITE(&st7789_handle, st7789_if_cmd_data_gpio_write);
  DRIVER_ST7789_LINK_RESET_GPIO_INIT(&st7789_handle, st7789_if_reset_gpio_init);
  DRIVER_ST7789_LINK_RESET_GPIO_DEINIT(&st7789_handle, st7789_if_reset_gpio_deinit);
  DRIVER_ST7789_LINK_RESET_GPIO_WRITE(&st7789_handle, st7789_if_reset_gpio_write);
  DRIVER_ST7789_LINK_DELAY_MS(&st7789_handle, st7789_if_delay_ms);
  DRIVER_ST7789_LINK_DEBUG_PRINT(&st7789_handle, st7789_if_debug_print);

  res = st7789_init(&st7789_handle);
  if(res != 0)
  {
    return res;
  }

  st7789_set_column(&st7789_handle, WK_ST7789_WIDTH);
  st7789_set_row(&st7789_handle, WK_ST7789_HEIGHT);
  st7789_set_memory_data_access_control(&st7789_handle, WK_ST7789_MADCTL_VALUE);
  st7789_set_interface_pixel_format(&st7789_handle, ST7789_RGB_INTERFACE_COLOR_FORMAT_65K, ST7789_CONTROL_INTERFACE_COLOR_FORMAT_16_BIT);
  st7789_sleep_out(&st7789_handle);
  /* most ST7789 IPS panels need inversion on for correct (non-negative) colors */
  st7789_display_inversion_on(&st7789_handle);
  st7789_normal_display_mode_on(&st7789_handle);

  wk_st7789_fill_screen(0x0000);

  st7789_display_on(&st7789_handle);
  gpio_bits_set(LED_EN_GPIO_PORT, LED_EN_PIN);

  return 0;
}

/**
  * @brief  fill a rectangle with an RGB565 color, chunked over dma.
  * @param  x0,y0,x1,y1: inclusive window, 0-indexed against
  *         WK_ST7789_WIDTH/HEIGHT
  * @param  color565: fill color
  * @retval none
  */
void wk_st7789_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color565)
{
  uint8_t chunk[WK_ST7789_FILL_CHUNK_PIXELS * 2];
  uint32_t total_pixels;
  uint32_t sent;
  uint32_t i;
  uint16_t this_chunk;

  if((x1 >= WK_ST7789_WIDTH) || (y1 >= WK_ST7789_HEIGHT) || (x0 > x1) || (y0 > y1))
  {
    return;
  }

  st7789_set_window(x0, y0, x1, y1);

  for(i = 0; i < WK_ST7789_FILL_CHUNK_PIXELS; i++)
  {
    chunk[2 * i]     = (uint8_t)(color565 >> 8);
    chunk[2 * i + 1] = (uint8_t)(color565 & 0xFF);
  }

  total_pixels = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);
  sent = 0;

  while(sent < total_pixels)
  {
    uint32_t remaining = total_pixels - sent;
    this_chunk = (uint16_t)((remaining > WK_ST7789_FILL_CHUNK_PIXELS) ? WK_ST7789_FILL_CHUNK_PIXELS : remaining);

    if(sent == 0)
    {
      st7789_memory_write(&st7789_handle, chunk, (uint16_t)(this_chunk * 2));
    }
    else
    {
      st7789_memory_continue_write(&st7789_handle, chunk, (uint16_t)(this_chunk * 2));
    }

    sent += this_chunk;
  }
}

/**
  * @brief  fill the entire visible screen with an RGB565 color.
  * @param  color565: fill color
  * @retval none
  */
void wk_st7789_fill_screen(uint16_t color565)
{
  wk_st7789_fill_rect(0, 0, WK_ST7789_WIDTH - 1, WK_ST7789_HEIGHT - 1, color565);
}
