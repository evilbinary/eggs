/* libdisplay-info 的替身实现（见 compat/libdisplay-info/info.h 的说明）。
 * headless 输出没有 EDID，weston_head_get_display_info() 恒 NULL，这些函数
 * 在运行期不会被调用；这里只为满足链接。 */
#include <libdisplay-info/info.h>

float
di_info_get_default_gamma(const struct di_info *info)
{
	(void)info;
	return 0.0f;
}

const struct di_color_primaries *
di_info_get_default_color_primaries(const struct di_info *info)
{
	(void)info;
	return 0;
}

const struct di_hdr_static_metadata *
di_info_get_hdr_static_metadata(const struct di_info *info)
{
	(void)info;
	return 0;
}
