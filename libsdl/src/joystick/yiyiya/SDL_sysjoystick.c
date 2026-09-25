/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    YiYiYa GPIO 手柄后端（SDL 1.2，miyoomini 等）。

    数据源同 SDL2 后端：内核 /dev/joystick（DEVICE_JOYSTICK），上报 Linux
    KEY_* 码流（每键 1 字节，release = code | 0x80）。
*/

#include "SDL_config.h"

#ifdef SDL_JOYSTICK_YIYIYA

/* This is the system specific header for the SDL joystick API */

#include "SDL_joystick.h"
#include "SDL_events.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define YIYIYA_JOY_DEV "/dev/joystick"
#define YIYIYA_JOY_NBUTTONS 16

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

/* Function to scan the system for joysticks.
 * 【SDL1.2 约定】返回可用 joystick 的【数量】：上层 SDL_JoystickInit() 会做
 * `SDL_numjoysticks = SDL_SYS_JoystickInit()`。返回 0 即"0 个设备"（这也是
 * dummy 后端的写法）。之前误返回 0 表示成功 ⇒ SDL_NumJoysticks() 恒为 0。 */
int SDL_SYS_JoystickInit(void)
{
	YIYIYA_joy_fd = open(YIYIYA_JOY_DEV, O_RDONLY);
	printf("YIYIYA joy init: open(%s) fd=%d\n", YIYIYA_JOY_DEV, YIYIYA_joy_fd);
	return (YIYIYA_joy_fd >= 0) ? 1 : 0;
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	if (index != 0) {
		SDL_SetError("No joystick available with that index");
		return (NULL);
	}
	return "YiYiYa GPIO Joystick";
}

/* Function to open a joystick for use. */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	if (YIYIYA_joy_fd < 0) {
		SDL_SetError("YiYiYa joystick not available");
		return (-1);
	}
	joystick->nbuttons = YIYIYA_JOY_NBUTTONS;
	joystick->naxes = 0;
	joystick->nhats = 0;
	return (0);
}

/* Function to update the state of a joystick - called as a device poll. */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	unsigned char buf[64];
	int n, i;
	static int dbg_read = 0;
	if (YIYIYA_joy_fd < 0) {
		return;
	}
	for (;;) {
		n = (int)read(YIYIYA_joy_fd, buf, sizeof(buf));
		if (dbg_read < 10) {
			dbg_read++;
			printf("YIYIYA read ret=%d\n", n);
		}
		if (n <= 0) {
			break;
		}
		for (i = 0; i < n; i++) {
			unsigned char c = buf[i];
			int pressed = !(c & 0x80);
			int k = c & 0x7F;
			int b = YIYIYA_KeyToButton(k);
			static int dbg_byte = 0;
			if (dbg_byte < 60) {
				dbg_byte++;
				printf("YIYIYA byte=%02x k=%d b=%d pressed=%d\n", c, k, b, pressed);
			}
			if (b >= 0) {
				SDL_PrivateJoystickButton(joystick, (Uint8)b,
				                          pressed ? SDL_PRESSED : SDL_RELEASED);
			}
		}
	}
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	return;
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	if (YIYIYA_joy_fd >= 0) {
		close(YIYIYA_joy_fd);
		YIYIYA_joy_fd = -1;
	}
	SDL_numjoysticks = 0;
}

#endif /* SDL_JOYSTICK_YIYIYA */
