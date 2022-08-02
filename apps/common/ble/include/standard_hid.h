#ifndef __STANDARD_HID_H__
#define __STANDARD_HID_H__

#include "typedef.h"

/*******************************************************************/
/*
 *-------------------hid for ble
 */
#define HID_UUID_16                                 0x1812
#define HID_INFORMATION_UUID_16                     0x2A4A
#define HID_REPORT_MAP_UUID_16                      0x2A4B
#define HID_CONTROL_POINT_UUID_16                   0x2A4C
#define HID_REPORT_UUID_16                          0x2A4D
#define PROTOCOL_MODE_UUID_16       				0x2A4E
#define HID_REPORT_REFERENCE_UUID_16                0x2908


/*******************************************************************/
/*
 *-------------------hid report(for slave)
 */
/*Main Items*/
#define INPUT(x)              (0x80 | 0x1), x
#define OUTPUT(x)             (0x90 | 0x1), x
#define COLLECTION(x)         (0xA0 | 0x1), x
#define FEATURE(x)            (0xB0 | 0x1), x
#define END_COLLECTION        0xC0
/*Golbal Items*/
#define USAGE_PAGE(x)         (0x04 | 0x1), x
#define LOGICAL_MIN(x)        (0x14 | 0x1), x
#define LOGICAL_MIN_16(x,y)   (0x14 | 0x2), x,y
#define LOGICAL_MAX(x)        (0x24 | 0x1), x
#define LOGICAL_MAX_16(x,y)   (0x24 | 0x2), x,y
#define PHYSICAL_MIN(x)       (0x34 | 0x1), x
#define PHYSICAL_MIN_16(x,y)  (0x34 | 0x2), x,y
#define PHYSICAL_MAX(x)       (0x44 | 0x1), x
#define PHYSICAL_MAX_16(x,y)  (0x44 | 0x2), x,y
#define UNIT_EXPONENT(x)      (0x54 | 0x1), x
#define UNIT(x)               (0x64 | 0x1), x
#define REPORT_SIZE(x)        (0x74 | 0x1), x
#define REPORT_ID(x)          (0x84 | 0x1), x
#define REPORT_COUNT(x)       (0x94 | 0x1), x
#define PUSH(x)               (0xA4 | 0x1), x
#define POP(x)                (0xB4 | 0x1), x
/*Local Items*/
#define USAGE(x)              (0x08 | 0x1), x
#define USAGE_16(x,y)         (0x08 | 0x2), x, y
#define USAGE_MIN(x)          (0x18 | 0x1), x
#define USAGE_MAX(x)          (0x28 | 0x1), x
#define USAGE_MAX_16(x,y)     (0x28 | 0x2), x,y
#define DESIGNATOR_INDEX(x)   (0x38 | 0x1), x
#define DESIGNATOR_MIN(x)     (0x48 | 0x1), x
#define DESIGNATOR_MAX(x)     (0x58 | 0x1), x
#define STRING_INDEX(x)       (0x78 | 0x1), x
#define STRING_MIN(x)         (0x88 | 0x1), x
#define STRING_MAX(x)         (0x98 | 0x1), x
#define DELIMITER(x)          (0xA8 | 0x1), x
/*Consumer Page*/
#define CONSUMER_PAGE           0x0C
#define DIGITIZERS              0x0D
// Usage ID
#define CONSUMER_CONTROL        0x01
#define GAME_PAD                0x05
#define Pen                     0x02
#define Touch_Screen            0x04

#define Stylus                  0x20
#define Finger                  0x22
#define Tip_Switch              0x42
#define Usage_X                 0x30
#define Usage_Y                 0x31
#define In_Range                0x32

#define POINTER                 0x01
#define MOUSE                   0x02
#define BUTTON                  0x09

#define ContactID               0x51
#define ContactCount            0x54
#define ContactMax              0x55
/*Generic Desktop Page*/
#define GENERIC_DESKTOP_PAGE    0x01
// Usage ID
#define POINTER                 0x01
#define DESKTOP_KEYBOARD        0x06
#define DESKTOP_KEYPAD          0x07
#define X_AXIS                  0x30
#define Y_AXIS                  0x31
#define Z_AXIS                  0x32
#define RZ_AXIS                 0x35
#define HAT_SWITCH              0x39
/*KEYBOARD/KEYPAD Page*/
#define KEYBOARD_PAGE           0x06
#define KEYBOARD_KEYPAD_PAGE    0x07
// Usage ID
#define KEYBOARD_A              0x04
#define KEYBOARD_RETURN         0x28
#define KEYBOARD_F19            0x6E
#define KEYBOARD_LEFTCONTROL    0xE0
#define KEYBOARD_RIGHT_GUI      0xE7
/*LED Page*/
#define LED_PAGE                0x08
// Usage ID
#define LED_NUM_LOCK            0x01
#define LED_KANA                0xE7
/*Button Page*/
#define BUTTON_PAGE             0x09
//Usage ID
#define BUTTON_1                0x01
#define BUTTON_12               0x0C
#define BUTTON_15               0x0F

//Collection
#define PHYSICAL                0x00
#define APPLICATION             0x01
#define LOGICAL                 0x02
#define REPORT                  0x03

#define PLAY                    0xB0
#define PAUSE                   0xB1
#define RECORD                  0xB2
#define FAST_FORWARD            0xB3
#define REWIND                  0xB4
#define SCAN_NEXT_TRACK         0xB5
#define SCAN_PREV_TRACK         0xB6
#define STOP                    0xB7

#define FRAME_FORWARD           0xC6
#define FRAME_BACK              0xC7
#define TRACKING_INC            0xCA
#define TRACKING_DEC            0xCB
#define STOP_EJECT              0xCC
#define PLAY_PAUSE              0xCD
#define PLAY_SKIP               0xCE

#define VOLUME                  0xE0
#define BALANCE                 0xE1
#define MUTE                    0xE2
#define BASS                    0xE3
#define VOLUME_INC              0xE9
#define VOLUME_DEC              0xEA

#define Menu                    0x40
#define Menu_Escape             0x46
#define AC_Home                 0x23, 0x02

#define BALANCE_LEFT            0x50, 0x01
#define BALANCE_RIGHT           0x51, 0x01
#define CHANNEL_LEFT            0x61, 0x01
#define CHANNEL_RIGHT           0x62, 0x01


/*******************************************************************/
/*
 *-------------------hid report resolve(for host)
 */
//main item
#define MAIN_ITEM           0
#define MAIN_ITEM_INPUT                 8
#define MAIN_ITEM_OUTPUT                9
#define MAIN_ITEM_COLLECTION            10
#define MAIN_ITEM_FEATURE               11
#define MAIN_ITEM_END_COLLECTION        12
//global item
#define GLOBAL_ITEM         1
#define GLOBAL_ITEM_USAGE_PAGE          0
#define GLOBAL_ITEM_LOGICAL_MINIMUM     1
#define GLOBAL_ITEM_LOGICAL_MAXIMUM     2
#define GLOBAL_ITEM_REPORT_SIZE         7
#define GLOBAL_ITEM_REPORT_ID           8
#define GLOBAL_ITEM_REPORT_COUNT        9
//local item
#define LOCAL_ITEM          2
#define LOCAL_ITEM_USAGE                0
#define LOCAL_ITEM_USAGE_MINIMUM        1
#define LOCAL_ITEM_USAGE_MAXIMUM        2

// item bit resolve
#define ITEM_TAG(x)                     ((x & 0xf0) >> 4)
#define ITEM_TYPE(x)                    ((x & 0x0c) >> 2)
#define ITEM_SIZE(x)                    (x & 0x03)

// keyboard usage page
#define KEYBOARD_USAGE_PAGE             0x07
#define NO_USAGE_PAGE                   0

// item type
#define SHORT_ITEM_STRUCT_SIZE          1
#define SHORT_ITEM_MAX_DATA_SIZE        sizeof(u32)

#endif // #define __STANDARD_HID_H__
