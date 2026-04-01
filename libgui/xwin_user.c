#include "xwin_user.h"

// ========== 用户空间实现 (示例) ==========


// Syscall 包装函数
long xwin_syscall(int num, long a1, long a2, long a3, long a4, long a5) {
  long ret;
#if defined(__aarch64__) || defined(ARM64)
  __asm__ volatile(
      "mov x8, %1\n"
      "mov x0, %2\n"
      "mov x1, %3\n"
      "mov x2, %4\n"
      "mov x3, %5\n"
      "mov x4, %6\n"
      "svc #0\n"
      "mov %0, x0\n"
      : "=r"(ret)
      : "r"(num), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
      : "x0", "x1", "x2", "x3", "x4", "x8");
#elif defined(__arm__) || defined(ARM)
  __asm__ volatile(
      "mov r7, %1\n"
      "mov r0, %2\n"
      "mov r1, %3\n"
      "mov r2, %4\n"
      "mov r3, %5\n"
      "mov r4, %6\n"
      "svc #0\n"
      "mov %0, r0\n"
      : "=r"(ret)
      : "r"(num), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
      : "r0", "r1", "r2", "r3", "r4", "r7");
#else
#error "Unsupported architecture for xwin_syscall"
#endif
  return ret;
}

xwin_handle_t xwin_create(i32 x, i32 y, u32 width, u32 height,
                          const char* title) {
  return (xwin_handle_t)xwin_syscall(SYS_XWIN_CREATE, (long)x, (long)y,
                                     (long)width, (long)height, (long)title);
}

void xwin_destroy(xwin_handle_t win) {
  xwin_syscall(SYS_XWIN_DESTROY, (long)win, 0, 0, 0, 0);
}

void xwin_move(xwin_handle_t win, i32 x, i32 y) {
  xwin_syscall(SYS_XWIN_MOVE, (long)win, (long)x, (long)y, 0, 0);
}

void xwin_resize(xwin_handle_t win, u32 width, u32 height) {
  xwin_syscall(SYS_XWIN_RESIZE, (long)win, (long)width, (long)height, 0, 0);
}

void xwin_show(xwin_handle_t win, int show) {
  xwin_syscall(SYS_XWIN_SHOW, (long)win, (long)show, 0, 0, 0);
}

void xwin_set_title(xwin_handle_t win, const char* title) {
  xwin_syscall(SYS_XWIN_SET_TITLE, (long)win, (long)title, 0, 0, 0);
}

void xwin_set_bg_color(xwin_handle_t win, u32 color) {
  xwin_syscall(SYS_XWIN_SET_BG_COLOR, (long)win, (long)color, 0, 0, 0);
}

void xwin_clear(xwin_handle_t win) {
  xwin_syscall(SYS_XWIN_CLEAR, (long)win, 0, 0, 0, 0);
}

void xwin_fill_rect(xwin_handle_t win, i32 x, i32 y, u32 w, u32 h, u32 color) {
  xwin_syscall(SYS_XWIN_FILL_RECT, (long)win, (long)x, (long)y,
               (long)w | ((long)h << 16), (long)color);
}

void xwin_draw_rect(xwin_handle_t win, i32 x, i32 y, u32 w, u32 h, u32 color) {
  xwin_syscall(SYS_XWIN_DRAW_RECT, (long)win, (long)x, (long)y,
               (long)w | ((long)h << 16), (long)color);
}

void xwin_draw_line(xwin_handle_t win, i32 x1, i32 y1, i32 x2, i32 y2,
                    u32 color) {
  xwin_syscall(SYS_XWIN_DRAW_LINE, (long)win, (long)x1 | ((long)y1 << 16),
               (long)x2 | ((long)y2 << 16), (long)color, 0);
}

void xwin_draw_text(xwin_handle_t win, i32 x, i32 y, const char* text,
                    u32 color) {
  xwin_syscall(SYS_XWIN_DRAW_TEXT, (long)win, (long)x, (long)y, (long)text,
               (long)color);
}

void xwin_blit(xwin_handle_t win, i32 x, i32 y, const u32* data, u32 w, u32 h) {
  xwin_syscall(SYS_XWIN_BLIT, (long)win, (long)x, (long)y,
               (long)w | ((long)h << 16), (long)data);
}

int xwin_get_event(void* event) {
  return (int)xwin_syscall(SYS_XWIN_GET_EVENT, (long)event, 0, 0, 0, 0);
}

void xwin_process_events(void) {
  xwin_syscall(SYS_XWIN_PROCESS_EVENTS, 0, 0, 0, 0, 0);
}

void xwin_render(void) { xwin_syscall(SYS_XWIN_RENDER, 0, 0, 0, 0, 0); }

void xwin_update(xwin_handle_t win) {
  xwin_syscall(SYS_XWIN_UPDATE, (long)win, 0, 0, 0, 0);
}
