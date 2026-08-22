/**
  **************************************************************************
  * @file     wk_task.c
  * @brief    global task dispatcher: calibration / normal / error state
  *           machine for the two hx71708 channels
  *
  * APP_STATE_CALIBRATION (initial state): each channel non-blockingly
  *   accumulates HX71708_CAL_SAMPLE_COUNT raw samples and averages them
  *   (sum / count) into its offset. once both channels have done so, the
  *   state machine promotes itself to APP_STATE_NORMAL. a watchdog bounds
  *   how long this is allowed to take, in case a channel's sensor never
  *   responds; if it fires first, the state machine falls to
  *   APP_STATE_ERROR instead.
  *
  * APP_STATE_NORMAL: each channel is refreshed as soon as its own
  *   conversion is ready; readings are exposed tare-corrected
  *   (raw - offset).
  *
  * APP_STATE_ERROR: terminal; nothing further is attempted.
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "wk_task.h"
#include "wk_system.h"

#define HX71708_CAL_SAMPLE_COUNT    (uint8_t)(20)
#define HX71708_CAL_WATCHDOG_US     (uint32_t)(3000000)  /* sensor-missing safety net */

typedef struct
{
  int32_t sum;
  uint8_t count;
  uint8_t done;
} cal_accum_type;

static app_state_type  app_state;
static cal_accum_type  cal_accum[HX71708_CH_NUM];
static int32_t         ch_offset[HX71708_CH_NUM];
static int32_t         ch_raw[HX71708_CH_NUM];
static uint32_t        cal_watchdog_ticks;

/**
  * @brief  calibration state: accumulate samples toward each channel's
  *         average offset; promote to normal once both are done, or to
  *         error if the watchdog fires first.
  * @param  none
  * @retval none
  */
static void state_calibration_run(void)
{
  uint8_t ch;

  for(ch = 0; ch < HX71708_CH_NUM; ch++)
  {
    int32_t sample;

    if(cal_accum[ch].done)
    {
      continue;
    }

    if(wk_hx71708_get_sample((hx71708_channel_type)ch, &sample))
    {
      cal_accum[ch].sum += sample;
      cal_accum[ch].count++;

      if(cal_accum[ch].count >= HX71708_CAL_SAMPLE_COUNT)
      {
        ch_offset[ch] = cal_accum[ch].sum / (int32_t)cal_accum[ch].count;
        cal_accum[ch].done = 1;
      }
    }
    else
    {
      wk_hx71708_check_stale((hx71708_channel_type)ch);
    }
  }

  if(cal_accum[HX71708_CH_LL].done && cal_accum[HX71708_CH_RR].done)
  {
    /* seed with the offset so the very first tared reading is 0 instead
       of -offset (ch_raw[] is still zero-init at this point; the real
       post-transition sample lands on the next state_normal_run() tick) */
    ch_raw[HX71708_CH_LL] = ch_offset[HX71708_CH_LL];
    ch_raw[HX71708_CH_RR] = ch_offset[HX71708_CH_RR];

    app_state = APP_STATE_NORMAL;
    return;
  }

  if(wk_tick_elapsed_us(&cal_watchdog_ticks, HX71708_CAL_WATCHDOG_US))
  {
    app_state = APP_STATE_ERROR;
  }
}

/**
  * @brief  normal state: keep each channel's raw reading fresh as soon as
  *         its own conversion becomes ready.
  * @param  none
  * @retval none
  */
static void state_normal_run(void)
{
  uint8_t ch;

  for(ch = 0; ch < HX71708_CH_NUM; ch++)
  {
    if(!wk_hx71708_get_sample((hx71708_channel_type)ch, &ch_raw[ch]))
    {
      wk_hx71708_check_stale((hx71708_channel_type)ch);
    }
  }
}

/**
  * @brief  reset the state machine to APP_STATE_CALIBRATION and clear
  *         accumulators.
  * @param  none
  * @retval none
  */
void wk_task_init(void)
{
  uint8_t ch;

  for(ch = 0; ch < HX71708_CH_NUM; ch++)
  {
    cal_accum[ch].sum = 0;
    cal_accum[ch].count = 0;
    cal_accum[ch].done = 0;
    ch_offset[ch] = 0;
    ch_raw[ch] = 0;
  }

  app_state = APP_STATE_CALIBRATION;

  /* seed the calibration watchdog reference to "now" */
  wk_tick_elapsed_us(&cal_watchdog_ticks, 0);
}

/**
  * @brief  run one step of the current state's work.
  * @param  none
  * @retval none
  */
void wk_task_dispatch(void)
{
  switch(app_state)
  {
    case APP_STATE_CALIBRATION:
      state_calibration_run();
      break;

    case APP_STATE_NORMAL:
      state_normal_run();
      break;

    case APP_STATE_ERROR:
    default:
      /* terminal: nothing to do */
      break;
  }
}

/**
  * @brief  current state as the protocol's status byte.
  * @param  none
  * @retval 0x00 normal, 0x01 calibrating, 0xFF error
  */
uint8_t wk_task_get_status_byte(void)
{
  switch(app_state)
  {
    case APP_STATE_NORMAL:
      return APP_STATUS_NORMAL;
    case APP_STATE_CALIBRATION:
      return APP_STATUS_CALIBRATING;
    case APP_STATE_ERROR:
    default:
      return APP_STATUS_FAILED;
  }
}

/**
  * @brief  calibration offset for ch.
  * @param  ch: hx71708 channel
  * @retval offset (0 until calibration has completed for this channel)
  */
int32_t wk_task_get_offset(hx71708_channel_type ch)
{
  return ch_offset[ch];
}

/**
  * @brief  tare-corrected reading for ch.
  * @param  ch: hx71708 channel
  * @param  value: filled with the tared reading, or 0 if not in normal state
  * @retval 1 if the state machine is in APP_STATE_NORMAL, 0 otherwise
  */
uint8_t wk_task_get_tared_reading(hx71708_channel_type ch, int32_t *value)
{
  if(app_state != APP_STATE_NORMAL)
  {
    *value = 0;
    return 0;
  }

  *value = ch_raw[ch] - ch_offset[ch];
  return 1;
}
