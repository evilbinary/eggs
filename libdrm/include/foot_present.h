/* foot_present.h - 默认 direct 的 GPU 上屏接口
 *
 * 应用只管把 RGBA8888 像素交进来；首次调用自动通过 gpud 查屏幕参数，
 * 默认 direct（mmap /dev/fb 直写）。需要 TFU 时显式传 FOOT_PRESENT_TFU。
 *
 * 输入布局：RGBA8888，row 0 = GL 窗口底部（glReadPixels 原始布局），
 * 内部按屏幕需要翻转行。
 */
#ifndef FOOT_PRESENT_H
#define FOOT_PRESENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FOOT_PRESENT_DEFAULT = -1, /* 首次调用时选 direct */
  FOOT_PRESENT_DIRECT = 0,   /* 直接写 /dev/fb */
  FOOT_PRESENT_TFU = 1       /* 离屏 BO -> TFU -> 屏幕 */
} foot_present_mode_t;

/* 默认 direct；w/h 为源图尺寸，居中贴到屏幕。返回 0 成功。 */
int foot_present_frame(const void* rgba, unsigned int w, unsigned int h);

/* 显式模式；mode 与已初始化模式不同时会重新初始化。 */
int foot_present_frame_ex(const void* rgba, unsigned int w, unsigned int h,
                          foot_present_mode_t mode);

/* 释放 /dev/fb 映射、离屏 BO 和 arena capability。 */
void foot_present_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
