/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

  YiYiYa GPIO 手柄后端（miyoomini 等）。

  数据源：内核 /dev/joystick（DEVICE_JOYSTICK）。内核上报 Linux KEY_* 码流，
  每个按键一个字节，release = code | 0x80（见 duck/modules/keyboard/keyboard.h
  以及 ssd202d.c / t113-s3.c）。

  SDL joystick 只提供 button（16 个），方向键也当 button；GetGamepadMapping()
  负责把 A/B/X/Y/BACK/START/L/R/DPAD 映射到对应 button，这样 SDL_GameController
  能直接识别，gnuboy/yui 的 gamepad 分支即可工作。
*/

#include "../../SDL_internal.h"

#if defined(SDL_JOYSTICK_YIYIYA)

#include "SDL_joystick.h"
#include "SDL_events.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define YIYIYA_JOY_DEV "/dev/joystick"
#define YIYIYA_JOY_NBUTTONS 16

/* Linux input KEY_* 码（与 duck/modules/keyboard/keyboard.h 对齐，不引用内核头） */
#define YK_UP 5
#define YK_DOWN 24
#define YK_LEFT 19
#define YK_RIGHT 4
#define YK_BUTTON_A 96
#define YK_BUTTON_B 97
#define YK_BUTTON_X 99
#define YK_BUTTON_Y 100
#define YK_BUTTON_L1 102
#define YK_BUTTON_R1 103
#define YK_BUTTON_L2 104
#define YK_BUTTON_R2 105
#define YK_BUTTON_SELECT 109
#define YK_BUTTON_START 108
#define YK_HOME 0xF0
#define YK_POWER 26

static int YIYIYA_joy_fd = -1;

static int
YIYIYA_KeyToButton(int k)
{
    switch (k) {
    case YK_UP:            return 0;
    case YK_DOWN:          return 1;
    case YK_LEFT:          return 2;
    case YK_RIGHT:         return 3;
    case YK_BUTTON_A:      return 4;
    case YK_BUTTON_B:      return 5;
    case YK_BUTTON_X:      return 6;
    case YK_BUTTON_Y:      return 7;
    case YK_BUTTON_L1:     return 8;
    case YK_BUTTON_R1:     return 9;
    case YK_BUTTON_L2:     return 10;
    case YK_BUTTON_R2:     return 11;
    case YK_BUTTON_SELECT: return 12;
    case YK_BUTTON_START:  return 13;
    case YK_HOME:          return 14;
    case YK_POWER:         return 15;
    default:               return -1;
    }
}

static int
YIYIYA_JoystickInit(void)
{
    YIYIYA_joy_fd = open(YIYIYA_JOY_DEV, O_RDONLY);
    printf("YIYIYA SDL2 joy init: open(%s) fd=%d\n", YIYIYA_JOY_DEV, YIYIYA_joy_fd);
    if (YIYIYA_joy_fd < 0) {
        return 0; /* 无手柄 */
    }
    return 0;
}

static int
YIYIYA_JoystickGetCount(void)
{
    return (YIYIYA_joy_fd >= 0) ? 1 : 0;
}

static void
YIYIYA_JoystickDetect(void)
{
}

static const char *
YIYIYA_JoystickGetDeviceName(int device_index)
{
    return "YiYiYa GPIO Joystick";
}

static int
YIYIYA_JoystickGetDevicePlayerIndex(int device_index)
{
    return -1;
}

static void
YIYIYA_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

static SDL_JoystickGUID
YIYIYA_JoystickGetDeviceGUID(int device_index)
{
    SDL_JoystickGUID guid;
    SDL_zero(guid);
    /* 固定前缀，便于 SDL 识别为同一设备 */
    guid.data[0] = 'Y';
    guid.data[1] = 'Y';
    guid.data[2] = 'J';
    guid.data[3] = 'S';
    return guid;
}

static SDL_JoystickID
YIYIYA_JoystickGetDeviceInstanceID(int device_index)
{
    return (SDL_JoystickID)0;
}

static int
YIYIYA_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    if (YIYIYA_joy_fd < 0) {
        return SDL_SetError("YiYiYa joystick not available");
    }
    joystick->nbuttons = YIYIYA_JOY_NBUTTONS;
    joystick->naxes = 0;
    joystick->nhats = 0;
    return 0;
}

static int
YIYIYA_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    return SDL_Unsupported();
}

static int
YIYIYA_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static Uint32
YIYIYA_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return 0;
}

static int
YIYIYA_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    return SDL_Unsupported();
}

static int
YIYIYA_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

static int
YIYIYA_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    return SDL_Unsupported();
}

static void
YIYIYA_JoystickUpdate(SDL_Joystick *joystick)
{
    unsigned char buf[64];
    int n, i;

    if (YIYIYA_joy_fd < 0) {
        return;
    }
    while ((n = (int)read(YIYIYA_joy_fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            unsigned char c = buf[i];
            int pressed = !(c & 0x80);
            int k = c & 0x7F;
            int b = YIYIYA_KeyToButton(k);
            static int dbg_byte = 0;
            if (dbg_byte < 60) {
                dbg_byte++;
                printf("YIYIYA2 byte=%02x k=%d b=%d pressed=%d\n", c, k, b, pressed);
            }
            if (b >= 0) {
                SDL_PrivateJoystickButton(joystick, (Uint8)b,
                                          pressed ? SDL_PRESSED : SDL_RELEASED);
            }
        }
    }
}

static void
YIYIYA_JoystickClose(SDL_Joystick *joystick)
{
}

static void
YIYIYA_JoystickQuit(void)
{
    if (YIYIYA_joy_fd >= 0) {
        close(YIYIYA_joy_fd);
        YIYIYA_joy_fd = -1;
    }
}

static SDL_bool
YIYIYA_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    out->a.kind = EMappingKind_Button; out->a.target = 4;
    out->b.kind = EMappingKind_Button; out->b.target = 5;
    out->x.kind = EMappingKind_Button; out->x.target = 6;
    out->y.kind = EMappingKind_Button; out->y.target = 7;
    out->back.kind = EMappingKind_Button; out->back.target = 12;
    out->start.kind = EMappingKind_Button; out->start.target = 13;
    out->leftshoulder.kind = EMappingKind_Button; out->leftshoulder.target = 8;
    out->rightshoulder.kind = EMappingKind_Button; out->rightshoulder.target = 9;
    out->dpup.kind = EMappingKind_Button; out->dpup.target = 0;
    out->dpdown.kind = EMappingKind_Button; out->dpdown.target = 1;
    out->dpleft.kind = EMappingKind_Button; out->dpleft.target = 2;
    out->dpright.kind = EMappingKind_Button; out->dpright.target = 3;
    return SDL_TRUE;
}

SDL_JoystickDriver SDL_YIYIYA_JoystickDriver =
{
    YIYIYA_JoystickInit,
    YIYIYA_JoystickGetCount,
    YIYIYA_JoystickDetect,
    YIYIYA_JoystickGetDeviceName,
    YIYIYA_JoystickGetDevicePlayerIndex,
    YIYIYA_JoystickSetDevicePlayerIndex,
    YIYIYA_JoystickGetDeviceGUID,
    YIYIYA_JoystickGetDeviceInstanceID,
    YIYIYA_JoystickOpen,
    YIYIYA_JoystickRumble,
    YIYIYA_JoystickRumbleTriggers,
    YIYIYA_JoystickGetCapabilities,
    YIYIYA_JoystickSetLED,
    YIYIYA_JoystickSendEffect,
    YIYIYA_JoystickSetSensorsEnabled,
    YIYIYA_JoystickUpdate,
    YIYIYA_JoystickClose,
    YIYIYA_JoystickQuit,
    YIYIYA_JoystickGetGamepadMapping
};

#endif /* SDL_JOYSTICK_YIYIYA */

/* vi: set ts=4 sw=4 expandtab: */
