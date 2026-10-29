/* 把内核的 /dev/input/event0（evdev）翻成 libweston 的 pointer 事件。
 * 本系统没有 libinput/udev，所以直接读事件节点做最小实现。 */
#ifndef YIYIYA_COMPAT_INPUT_H
#define YIYIYA_COMPAT_INPUT_H

struct weston_compositor;

/* 返回 0 成功；没有 seat 或打不开 event0 时返回 -1 */
int yy_input_init(struct weston_compositor *ec);

/* 把指针挪到输出坐标 (x,y)（超出范围的会被 libweston clamp） */
void yy_input_move_to(double x, double y);

/* 当前指针位置（输出坐标）；未初始化时返回 0 */
void yy_input_get_pos(double *x, double *y);

#endif /* YIYIYA_COMPAT_INPUT_H */
