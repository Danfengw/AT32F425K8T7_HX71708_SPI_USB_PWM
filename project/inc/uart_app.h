/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __UART_APP_H
#define __UART_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "at32f425_wk_config.h"

/**
  * frame format (controller comm protocol, "涛涛控制器通讯协议V1.1.29"):
  *
  *   Data[0]     frame header, fixed 0x5A
  *   Data[1]     device address / permission byte (see uart_app_build_address())
  *   Data[2]     function code low byte
  *   Data[3]     function code high byte (always 0x00 in practice; every
  *               defined function code is <= 0x00FF)
  *   Data[4]     data length N (0..UART_APP_MAX_DATA_LEN)
  *   Data[5..]   N bytes of data
  *   Data[5+N]   CRC16 high byte
  *   Data[5+N+1] CRC16 low byte  (crc covers Data[0] .. Data[4+N] inclusive)
  *
  * multi-byte fields elsewhere in the protocol are big-endian (high byte
  * first, low byte after), so the CRC16 is transmitted the same way.
  */

/* exported constants --------------------------------------------------------*/
#define UART_APP_FRAME_HEADER          (uint8_t)(0x5A)
#define UART_APP_MAX_DATA_LEN          (130)
#define UART_APP_PREFIX_LEN            (5)   /* header+addr+funclo+funchi+len */
#define UART_APP_MAX_FRAME_LEN         (UART_APP_PREFIX_LEN + UART_APP_MAX_DATA_LEN + 2)

/* Data[1] bit7: primary/secondary role */
#define UART_APP_ADDR_ROLE_MASTER      (uint8_t)(0x00)
#define UART_APP_ADDR_ROLE_SLAVE       (uint8_t)(0x80)

/* Data[1] bit4-6: read/write permission */
typedef enum
{
  UART_APP_PERM_READ_ONLY    = 0,  /* 000: read only */
  UART_APP_PERM_WRITE_NO_ACK = 1,  /* 001: write, no ack expected */
  UART_APP_PERM_WRITE_ACK    = 2,  /* 010: write, ack expected */
  UART_APP_PERM_ACK          = 3   /* 011: this frame is itself the ack/response */
} uart_app_perm_type;

/* Data[1] bit0-3: device id (only these three are defined so far) */
#define UART_APP_DEV_CONTROLLER        (uint8_t)(0x01)
#define UART_APP_DEV_BATTERY           (uint8_t)(0x02)
#define UART_APP_DEV_METER             (uint8_t)(0x03)

/* function codes (protocol section 2) */
#define UART_APP_FUNC_CONTROLLER_TYPE            (uint16_t)(0x0000)
#define UART_APP_FUNC_HEARTBEAT                  (uint16_t)(0x0001)
#define UART_APP_FUNC_CONTROLLER_STATUS          (uint16_t)(0x0002)
#define UART_APP_FUNC_PARAM_CONFIG               (uint16_t)(0x0003)
#define UART_APP_FUNC_LIGHT_CONTROL              (uint16_t)(0x0004)
#define UART_APP_FUNC_GEAR_SPEED                 (uint16_t)(0x0005)
#define UART_APP_FUNC_AUTO_SHUTDOWN_TIME         (uint16_t)(0x0006)
#define UART_APP_FUNC_REALTIME_CLOCK             (uint16_t)(0x0007)
#define UART_APP_FUNC_TRIP_DISTANCE              (uint16_t)(0x0008)
#define UART_APP_FUNC_TOTAL_DISTANCE             (uint16_t)(0x0009)
#define UART_APP_FUNC_TEMPERATURE                (uint16_t)(0x000A)
#define UART_APP_FUNC_CURRENT                    (uint16_t)(0x000B)
#define UART_APP_FUNC_BATTERY_VOLTAGE            (uint16_t)(0x000C)
#define UART_APP_FUNC_BATTERY_CAPACITY_TOTAL     (uint16_t)(0x000D)
#define UART_APP_FUNC_BATTERY_CAPACITY_REMAIN    (uint16_t)(0x000E)
#define UART_APP_FUNC_BATTERY_CYCLE_COUNT        (uint16_t)(0x000F)
#define UART_APP_FUNC_BATTERY_VENDOR_INFO        (uint16_t)(0x0010)
#define UART_APP_FUNC_CONTROLLER_VENDOR_INFO     (uint16_t)(0x0011)
#define UART_APP_FUNC_GEARED_MOTOR_HALL_PULSES   (uint16_t)(0x0012)
#define UART_APP_FUNC_GEARED_MOTOR_HALL_INTERVAL (uint16_t)(0x0013)
#define UART_APP_FUNC_MOTOR_PARAMS               (uint16_t)(0x0014)
#define UART_APP_FUNC_WHEEL_DIAMETER              (uint16_t)(0x0015)
#define UART_APP_FUNC_FLASH_DATA_VERSION         (uint16_t)(0x0016)
#define UART_APP_FUNC_VENDOR_SW_HW_VERSION       (uint16_t)(0x0017)
#define UART_APP_FUNC_BACKLIGHT_BRIGHTNESS       (uint16_t)(0x0018)
#define UART_APP_FUNC_NIGHT_MODE                 (uint16_t)(0x0019)
#define UART_APP_FUNC_RGB_LIGHT                  (uint16_t)(0x001A)
#define UART_APP_FUNC_SW_SIGNATURE               (uint16_t)(0x001B)
#define UART_APP_FUNC_SN_NUMBER                  (uint16_t)(0x001D)
#define UART_APP_FUNC_DEVICE_INFO_STRING         (uint16_t)(0x001E)
#define UART_APP_FUNC_BATTERY_STATUS             (uint16_t)(0x0020)
#define UART_APP_FUNC_GYRO_ANGLE                 (uint16_t)(0x0021)
#define UART_APP_FUNC_RESPONSE_TIME_CONFIG       (uint16_t)(0x0022)
#define UART_APP_FUNC_BMS_FW_VERSION              (uint16_t)(0x002A)
#define UART_APP_FUNC_REMAINING_RANGE             (uint16_t)(0x0030)
#define UART_APP_FUNC_TRIP_TIME                   (uint16_t)(0x0031)
#define UART_APP_FUNC_SPEED_STATS                 (uint16_t)(0x0032)
#define UART_APP_FUNC_INCOMING_CALL_PUSH          (uint16_t)(0x00A0)
#define UART_APP_FUNC_SMS_PUSH                    (uint16_t)(0x00A1)
#define UART_APP_FUNC_NOTIFICATION_PUSH           (uint16_t)(0x00A2)
#define UART_APP_FUNC_TIME_PUSH                   (uint16_t)(0x00A3)
#define UART_APP_FUNC_BLUETOOTH_PASSWORD          (uint16_t)(0x00A4)
#define UART_APP_FUNC_NAVIGATION_PUSH             (uint16_t)(0x00A5)
#define UART_APP_FUNC_FACTORY_RESET               (uint16_t)(0x00A6)
#define UART_APP_FUNC_LOCK_UNLOCK                 (uint16_t)(0x00A7)
#define UART_APP_FUNC_LOCK_PASSWORD_CHANGE        (uint16_t)(0x00A8)
#define UART_APP_FUNC_SERVICE_PASSWORD_QUERY      (uint16_t)(0x00A9)
#define UART_APP_FUNC_WEATHER_PUSH                 (uint16_t)(0x00AA)
#define UART_APP_FUNC_BOOTLOADER_UPGRADE_QUERY     (uint16_t)(0x00D0)
#define UART_APP_FUNC_BOOTLOADER_UPGRADE_DATA      (uint16_t)(0x00D1)
#define UART_APP_FUNC_BOOTLOADER_UPGRADE_VERIFY    (uint16_t)(0x00D2)
#define UART_APP_FUNC_APPLICATION_UPGRADE_QUERY    (uint16_t)(0x00E0)
#define UART_APP_FUNC_APPLICATION_UPGRADE_DATA     (uint16_t)(0x00E1)
#define UART_APP_FUNC_APPLICATION_UPGRADE_VERIFY   (uint16_t)(0x00E2)
#define UART_APP_FUNC_FLASH_UPGRADE_QUERY          (uint16_t)(0x00F0)
#define UART_APP_FUNC_FLASH_UPGRADE_DATA           (uint16_t)(0x00F1)
#define UART_APP_FUNC_FLASH_UPGRADE_VERIFY         (uint16_t)(0x00F2)

/* exported types -------------------------------------------------------------*/
typedef struct
{
  uint8_t  address;                       /* raw Data[1] byte */
  uint16_t func_code;                     /* Data[2] | (Data[3] << 8) */
  uint8_t  data[UART_APP_MAX_DATA_LEN];
  uint8_t  data_len;
} uart_app_frame_type;

/* exported functions ------------------------------------------------------- */

  /* crc16 over data[0..len-1]. exact reference algorithm from the protocol
     spec (bit-manipulation, not table-based) - keep bit-for-bit identical
     to interoperate with the host/other nodes. */
  uint16_t uart_app_crc16(const uint8_t *data, uint16_t len);

  /* pack role/permission/device-id into a Data[1] address byte. */
  uint8_t uart_app_build_address(uint8_t is_slave, uart_app_perm_type permission, uint8_t device_id);

  /* encode a complete frame (header..crc16) into out_buf, which must be at
     least UART_APP_MAX_FRAME_LEN bytes. returns the total frame length in
     bytes, or 0 if data_len exceeds UART_APP_MAX_DATA_LEN. */
  uint16_t uart_app_encode_frame(uint8_t *out_buf, uint8_t address, uint16_t func_code,
                                  const uint8_t *data, uint8_t data_len);

  /* feed newly-arrived raw bytes (e.g. from a uart rx driver) into the
     parser. resyncs on the 0x5A header and validates crc16 before exposing
     a frame; malformed/corrupt bytes in between are silently discarded. */
  void uart_app_parse_feed(const uint8_t *chunk, uint16_t len);

  /* 1 if a validated frame is waiting to be consumed, 0 otherwise. */
  uint8_t uart_app_frame_ready(void);

  /* copies the ready frame out and clears the ready flag. returns 1 if a
     frame was actually copied, 0 if none was ready. */
  uint8_t uart_app_get_frame(uart_app_frame_type *out_frame);

#ifdef __cplusplus
}
#endif

#endif
