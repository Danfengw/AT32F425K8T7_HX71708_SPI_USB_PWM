/**
  **************************************************************************
  * @file     wk_motor.c
  * @brief    motor-enable squeeze switch (MOTOR_EN_SW / H3)
  *
  * MOTOR_EN_SW (PA15) is an unlatched momentary switch on H3 (the
  * handle/grip trigger): squeezing it closes the contact, releasing it
  * opens it again. There's no long-press behavior here, just
  * press/release, serviced by the same generic wk_button_service() state
  * machine BTN_1 uses (wk_button.h) - only on_press/on_release are wired
  * up, on_short_press/on_long_press are unused.
  *
  * Intended behavior (not implemented yet - see DEVELOPMENT_LOG.md
  * "Motor controller integration test"):
  *   - squeeze H3 while the hx71708 has a valid reading -> motor enables
  *   - release H3 (hx71708 reading or not) -> motor stops
  * The callbacks below are placeholders for that dispatch logic.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wk_motor.h"
#include "wk_button.h"
#include "at32f425_wk_config.h"

/* long_press_us is unused (no on_long_press handler registered for this
   switch), set high enough to never matter */
#define WK_MOTOR_LONG_PRESS_UNUSED_US    (uint32_t)(0xFFFFFFFF)

static wk_button_type motor_en_btn;

/**
  * @brief  fires once when H3 is first squeezed.
  * @note   TODO (motor dispatch not implemented yet): only actually
  *         enable the motor if the hx71708 currently has a valid reading
  *         (see wk_task_get_tared_reading()) - squeezing H3 with no
  *         load-cell reading should not enable the motor. Wire the
  *         actual motor drive gpio/pwm here.
  * @param  none
  * @retval none
  */
static void wk_motor_on_squeeze(void)
{
  /* TODO: if hx71708 has a valid reading, enable motor */
}

/**
  * @brief  fires once when H3 is released.
  * @note   TODO (motor dispatch not implemented yet): stop the motor
  *         unconditionally here, regardless of hx71708 reading state.
  * @param  none
  * @retval none
  */
static void wk_motor_on_release(void)
{
  /* TODO: stop motor */
}

/**
  * @brief  arm MOTOR_EN_SW's press/release callbacks.
  * @param  none
  * @retval none
  */
void wk_motor_init(void)
{
  motor_en_btn.port = MOTOR_EN_SW_GPIO_PORT;
  motor_en_btn.pin = MOTOR_EN_SW_PIN;
  motor_en_btn.long_press_us = WK_MOTOR_LONG_PRESS_UNUSED_US;
  motor_en_btn.on_press = wk_motor_on_squeeze;
  motor_en_btn.on_release = wk_motor_on_release;
  motor_en_btn.on_short_press = 0;
  motor_en_btn.on_long_press = 0;
  motor_en_btn.state = WK_BUTTON_IDLE;
  motor_en_btn.press_ticks = 0;
}

/**
  * @brief  service MOTOR_EN_SW's debounce state machine.
  * @param  none
  * @retval none
  */
void wk_motor_task(void)
{
  wk_button_service(&motor_en_btn);
}
