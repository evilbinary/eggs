/* frontend 辅助符号的最小实现（静态链接版 weston 用）：
 *   wet_get_config / wet_get_libexec_path / wet_client_start / wet_shell_init
 *   screenshooter_create
 * upstream 这些在 frontend/main.c 里，依赖模块加载、session、xwayland 等一大坨；
 * 这里只保留 shell 真正用到的语义。 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../libweston/libweston-internal.h"
#include "wet-frontend.h"

/* 桌面 shell 的装饰/面板客户端：由我们的 app/wayland/desktop-ui 提供 */
#define YY_DESKTOP_SHELL_PATH "/bin/weston-desktop-ui"

struct weston_config *
wet_get_config(struct weston_compositor *compositor)
{
	struct yy_compositor *c = (struct yy_compositor *)compositor;

	return c->config;
}

static char *
yy_path_for(const char *name)
{
	char buf[128];

	/* upstream 的默认名是 "weston-desktop-shell"：映射到我们自己的面板客户端 */
	if (name == NULL || strstr(name, "desktop-shell") != NULL)
		return strdup(YY_DESKTOP_SHELL_PATH);

	snprintf(buf, sizeof(buf), "/bin/%s", name);
	return strdup(buf);
}

char *
wet_get_libexec_path(const char *name)
{
	return yy_path_for(name);
}

char *
wet_get_bindir_path(const char *name)
{
	return yy_path_for(name);
}

/* fork 一个客户端进程，把它和 compositor 用 socketpair 直接连起来
 * （WAYLAND_SOCKET 传 fd，不经过 socket 文件）。 */
struct wl_client *
wet_client_start(struct weston_compositor *compositor, const char *path)
{
	int sv[2];
	pid_t pid;
	struct wl_client *client;
	char fdstr[16];

	if (path == NULL)
		return NULL;
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		printf("FRONTEND: socketpair 失败: %s\n", strerror(errno));
		fflush(stdout);
		return NULL;
	}

	pid = fork();
	if (pid < 0) {
		printf("FRONTEND: fork 失败: %s\n", strerror(errno));
		fflush(stdout);
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	if (pid == 0) {
		/* 子进程：WAYLAND_SOCKET 必须是可继承的 fd */
		close(sv[0]);
		fcntl(sv[1], F_SETFD, 0);
		snprintf(fdstr, sizeof(fdstr), "%d", sv[1]);
		setenv("WAYLAND_SOCKET", fdstr, 1);
		execl(path, path, (char *)NULL);
		_exit(127);
	}

	close(sv[1]);
	client = wl_client_create(compositor->wl_display, sv[0]);
	if (client == NULL) {
		printf("FRONTEND: wl_client_create 失败（%s）\n", path);
		fflush(stdout);
		close(sv[0]);
		return NULL;
	}

	printf("FRONTEND: 已启动客户端 %s (pid=%d)\n", path, (int)pid);
	fflush(stdout);
	return client;
}

int
wet_shell_init(struct weston_compositor *ec, int *argc, char *argv[])
{
	(void)ec;
	(void)argc;
	(void)argv;
	return 0;
}

void
screenshooter_create(struct weston_compositor *ec)
{
	(void)ec;
}
