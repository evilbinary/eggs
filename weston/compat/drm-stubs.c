/* libdrm / cairo 相关的最小替身实现。
 *
 * 1) drmGetFormatModifierVendor / drmGetFormatModifierName
 *    libweston/pixel-formats.c 把它们的结果拼进格式名字符串（日志/调试用）。
 *    本系统没有 DRM 设备，也没有 libdrm，返回空即可（调用处按 NULL 处理）。
 *
 * 2) weston_renderer_borders_update / _fini
 *    真实的实现在 libweston/renderer-borders.c，靠 cairo 画窗口边框阴影。
 *    无 cairo 且 headless 输出不上屏，用空实现顶掉，接口语义不变。
 */
#include <stdint.h>

#include "renderer-borders.h"

const char *
drmGetFormatModifierVendor(uint64_t modifier)
{
	(void)modifier;
	return NULL;
}

const char *
drmGetFormatModifierName(uint64_t modifier)
{
	(void)modifier;
	return NULL;
}

void
weston_renderer_borders_update(struct weston_renderer_borders *borders,
			       struct frame *frame,
			       struct weston_output *output)
{
	(void)borders;
	(void)frame;
	(void)output;
}

void
weston_renderer_borders_fini(struct weston_renderer_borders *borders,
			     struct weston_output *output)
{
	(void)borders;
	(void)output;
}
