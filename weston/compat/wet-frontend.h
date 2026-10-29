/* weston 的 shell 通过这几个 frontend 符号拿配置 / 启动客户端。
 * 我们走静态链接（没有 .so 插件机制），所以这里给一份最小实现。 */
#ifndef YIYIYA_WET_FRONTEND_H
#define YIYIYA_WET_FRONTEND_H

#include <libweston/libweston.h>
#include <libweston/config-parser.h>

/* 挂在 weston_compositor 的 user_data 上（与 upstream frontend 的
 * to_wet_compositor 一致：weston_compositor_get_user_data）。 */
struct yy_compositor {
	struct weston_config *config;
};

struct wl_client *
wet_client_start(struct weston_compositor *compositor, const char *path);

struct weston_config *
wet_get_config(struct weston_compositor *compositor);

char *
wet_get_libexec_path(const char *name);

char *
wet_get_bindir_path(const char *name);

void
screenshooter_create(struct weston_compositor *ec);

#endif /* YIYIYA_WET_FRONTEND_H */
