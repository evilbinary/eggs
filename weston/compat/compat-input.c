/* evdev → wl_pointer。
 *
 * 本系统没有 libinput/udev（没有 /sys 枚举、没有 udev 规则），所以直接读内核的
 * evdev 节点：EV_REL 的增量累加成"绝对坐标"（合成器要的是全局坐标），
 * EV_KEY 的 BTN_* 原样透传，SYN_REPORT 时把这一批事件作为一个 pointer frame
 * 送给客户端。事件源挂进 wl_event_loop，来事件就立刻注入。 */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../libweston/libweston-internal.h"
#include "compat-input.h"

#define EV_SYN 0
#define EV_KEY 1
#define EV_REL 2
#define REL_X 0
#define REL_Y 1
#define SYN_REPORT 0
#define BTN_LEFT_K 0x110

struct input_event_k {
	int64_t sec;
	int64_t usec;
	uint16_t type;
	uint16_t code;
	int32_t value;
};

static struct {
	int fd;
	struct wl_event_source *src;
	struct weston_pointer *pointer;
	double x;
	double y;
} input = { .fd = -1 };

static struct timespec
input_now(void)
{
	struct timespec ts = { 0, 0 };

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts;
}

void
yy_input_move_to(double x, double y)
{
	input.x = x;
	input.y = y;
}

void
yy_input_get_pos(double *x, double *y)
{
	if (x != NULL)
		*x = input.x;
	if (y != NULL)
		*y = input.y;
}

/* 把当前位置作为一个绝对坐标 motion 事件注入（内部会 pick view 并发
 * enter/leave/motion） */
static void
input_flush_motion(void)
{
	struct weston_pointer_motion_event ev;
	struct weston_coord_global abs;
	struct timespec ts = input_now();

	if (input.pointer == NULL)
		return;

	abs.c.x = input.x;
	abs.c.y = input.y;
	weston_pointer_motion_event_init(&ev, &ts, input.pointer->seat,
					 WESTON_POINTER_MOTION_ABS, &abs, NULL,
					 NULL);
	weston_pointer_send_motion(input.pointer, &ev);
	weston_pointer_send_frame(input.pointer);
}

static void
input_button(uint32_t button, int pressed)
{
	struct weston_pointer_button_event ev;
	struct timespec ts = input_now();

	if (input.pointer == NULL)
		return;

	weston_pointer_button_event_init(&ev, &ts, input.pointer->seat, button,
					 pressed ? WL_POINTER_BUTTON_STATE_PRESSED
						 : WL_POINTER_BUTTON_STATE_RELEASED);
	weston_pointer_send_button(input.pointer, &ev);
	weston_pointer_send_frame(input.pointer);
}

static int
input_dispatch(int fd, uint32_t mask, void *data)
{
	struct input_event_k ev;
	int moved = 0;
	int n;

	(void)mask;
	(void)data;

	while ((n = read(fd, &ev, sizeof(ev))) == (int)sizeof(ev)) {
		if (ev.type == EV_REL) {
			if (ev.code == REL_X) {
				input.x += (double)ev.value;
				moved = 1;
			} else if (ev.code == REL_Y) {
				input.y += (double)ev.value;
				moved = 1;
			}
		} else if (ev.type == EV_KEY) {
			if (ev.code == BTN_LEFT_K)
				input_button(BTN_LEFT_K, ev.value != 0);
		} else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
			if (moved) {
				moved = 0;
				input_flush_motion();
			}
		}
	}
	return 0;
}

int
yy_input_init(struct weston_compositor *ec)
{
	struct wl_event_loop *loop = wl_display_get_event_loop(ec->wl_display);
	struct weston_seat *seat = NULL;
	struct weston_output *output = NULL;

	if (!wl_list_empty(&ec->seat_list))
		seat = wl_container_of(ec->seat_list.next, seat, link);
	if (seat == NULL || seat->pointer_state == NULL) {
		printf("INPUT: 没有带 pointer 的 seat，鼠标不可用\n");
		fflush(stdout);
		return -1;
	}
	input.pointer = seat->pointer_state;

	if (!wl_list_empty(&ec->output_list))
		output = wl_container_of(ec->output_list.next, output, link);

	/* 起始位置放输出中间 */
	if (output != NULL) {
		input.x = output->width / 2.0;
		input.y = output->height / 2.0;
	} else {
		input.x = 320;
		input.y = 240;
	}

	input.fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
	if (input.fd < 0) {
		printf("INPUT: 打不开 /dev/input/event0，鼠标不可用\n");
		fflush(stdout);
		return -1;
	}

	input.src = wl_event_loop_add_fd(loop, input.fd, WL_EVENT_READABLE,
					 input_dispatch, NULL);
	if (input.src == NULL) {
		printf("INPUT: event0 挂进事件循环失败\n");
		fflush(stdout);
		close(input.fd);
		input.fd = -1;
		return -1;
	}

	printf("INPUT: /dev/input/event0 → wl_pointer 就绪（%dx%d 起）\n",
	       (int)input.x, (int)input.y);
	fflush(stdout);
	return 0;
}
