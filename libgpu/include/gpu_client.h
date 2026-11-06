/* gpu_client.h - YiYiYa 客户端 GPU 接口（对接 gpud）
 *
 * 应用/EGL/weston 只通过这里和 gpud 打交道；libdrm 里不再放 present 逻辑。
 */
#ifndef GPU_CLIENT_H
#define GPU_CLIENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned int gva, pa, width, height, pitch, cpp, format, size;
} gpu_screen_info_t;

typedef enum {
  GPU_PRESENT_DEFAULT = -1, /* 首次调用时选 direct */
  GPU_PRESENT_DIRECT = 0,
  GPU_PRESENT_TFU = 1
} gpu_present_mode_t;

/* 查询屏幕几何；成功返回 0。 */
int gpu_screen_info(gpu_screen_info_t* info);

/* 把一个已存在的物理块（如 foot arena）导入 gpud，返回 BO 句柄。 */
int gpu_bo_create(unsigned int pa, unsigned int size, unsigned int* handle);
int gpu_bo_destroy(unsigned int handle);

/* 提交一个已有 BO 到屏幕。
 * src_fmt: 1 = RGB565，其余按 ARGB8888；src_stride 字节数，0 = 紧凑。
 * dst_x/dst_y 是屏幕坐标。 */
int gpu_present_bo(unsigned int handle, unsigned int w, unsigned int h,
                   unsigned int src_fmt, unsigned int src_stride,
                   unsigned int dst_x, unsigned int dst_y, int mode);

/* 把 RGBA8888（row 0 = GL 底部）转 RGB565 后上屏。 */
int gpu_present_frame(const void* rgba, unsigned int w, unsigned int h);
int gpu_present_frame_ex(const void* rgba, unsigned int w, unsigned int h,
                         gpu_present_mode_t mode);

/* 释放内部的/离屏 BO。 */
void gpu_present_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
