/* libdisplay-info 的最小替身头。
 *
 * 上游 weston 用它在【DRM 后端】读 EDID（色域/伽马/HDR 静态元数据）。
 * 本移植不编 DRM 后端（无 /dev/dri），headless 后端的 head 也没有 EDID：
 * weston_head_get_display_info() 恒返回 NULL，所以这里只需要让
 * frontend/main.c 编译通过、链接期给出同名符号，运行期永不进入。
 */
#ifndef YIYIYA_LIBDISPLAY_INFO_STUB_H
#define YIYIYA_LIBDISPLAY_INFO_STUB_H

#include <stdbool.h>

/* 不透明句柄：只要指针类型对得上就行 */
struct di_info;

struct di_xy {
	float x;
	float y;
};

struct di_color_primaries {
	bool has_primaries;
	bool has_default_white_point;
	struct di_xy primary[3];
	struct di_xy default_white;
};

struct di_hdr_static_metadata {
	float desired_content_min_luminance;
	float desired_content_max_luminance;
	float desired_content_max_frame_avg_luminance;
};

float di_info_get_default_gamma(const struct di_info *info);
const struct di_color_primaries *
di_info_get_default_color_primaries(const struct di_info *info);
const struct di_hdr_static_metadata *
di_info_get_hdr_static_metadata(const struct di_info *info);

#endif /* YIYIYA_LIBDISPLAY_INFO_STUB_H */
