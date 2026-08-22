/**
  **************************************************************************
  * @file     wk_motor.h
  * @brief    motor-enable squeeze switch (MOTOR_EN_SW / H3)
  **************************************************************************
  */
#ifndef __WK_MOTOR_H
#define __WK_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

  /* arm MOTOR_EN_SW's press/release callbacks. Call once from main(). */
  void wk_motor_init(void);

  /* service MOTOR_EN_SW's debounce state machine. Call every main loop
     iteration. */
  void wk_motor_task(void);

#ifdef __cplusplus
}
#endif

#endif
