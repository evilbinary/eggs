/* <linux/input.h> 垫片。
 *
 * weston 前端直接 include <linux/input.h>（EV_KEY/KEY_* 等）。仓库里有多份
 * 同名 uapi 头（libevdev / libinput / libudev / libmtdev），include 顺序不
 * 稳定时可能命中 libinput 的 wrapper：它在 !__linux__ 下什么都不定义，
 * 于是 KEY_S/EV_KEY 全变未定义。这里固定转发到 libevdev 的完整 uapi 拷贝，
 * 和 libweston 实际使用的输入栈保持一致。
 */
#ifndef YIYIYA_WESTON_COMPAT_LINUX_INPUT_H
#define YIYIYA_WESTON_COMPAT_LINUX_INPUT_H

#include "../../../libevdev/include/linux/input.h"

#endif /* YIYIYA_WESTON_COMPAT_LINUX_INPUT_H */
