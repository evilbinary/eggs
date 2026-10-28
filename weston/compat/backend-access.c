/* 给应用侧（app/wayland）用的后端访问小工具。
 *
 * libweston 的公开头文件没有"遍历已加载后端"的 API：正常情况下前端通过
 * weston_compositor_load_backend() 的返回值拿到 struct weston_backend *。
 * 我们静态链接、直接调后端的 weston_backend_init()，拿不到返回值，因此
 * 在这里用私有头（compositor.h 里的 backend_list）补一个取值函数。
 */
#include "../libweston/libweston-internal.h"
#include "../libweston/backend.h"

struct weston_backend *
weston_compositor_get_first_backend(struct weston_compositor *compositor)
{
	struct weston_backend *backend;

	wl_list_for_each(backend, &compositor->backend_list, link)
		return backend;

	return NULL;
}
