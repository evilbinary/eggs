# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# weston 16.0.1 的 libweston（静态库，阶段 1：headless + pixman 渲染器）。
#
# 不走 meson：config.h / git-version.h / include/libweston/version.h 手工给，
# 协议代码用 eggs/wayland-protocols/gen/ 里预生成的（见其 gen.sh）。
#
# 内容 = 上游 libweston/meson.build 的 srcs_libweston + libweston-desktop +
# shell-utils + backend-headless/headless.c + libshared 的构建期必需子集。
#
# 裁掉的东西（阶段 1 不需要，且各自拖大依赖）：
#   renderer-gl（EGL/GLES）、renderer-vulkan、backend-{drm,wayland,x11,
#   pipewire,rdp,vnc}、color-lcms（lcms2）、clients/（cairo/pango）、tests。
#
# 依赖：wayland（client+server）、pixman、xkbcommon。
target("weston")
set_kind("static")

add_packages(get_config('default_libc'))

add_deps("wayland", "pixman", "xkbcommon")
# 只为 <linux/input.h>（compositor.c / input.c 用 input_event 等结构）；
# libevdev 自身不会被链接进来（没有符号被引用）。
add_deps("evdev")

add_files(
    # ---- 缺失依赖的替身（libdrm 名字查询、cairo 窗口边框）----
    'compat/drm-stubs.c',
    # ---- libweston 核心 ----
    'libweston/alpha-modifier.c',
    'libweston/animation.c',
    'libweston/auth.c',
    'libweston/bindings.c',
    'libweston/clipboard.c',
    'libweston/color.c',
    'libweston/color-properties.c',
    'libweston/color-management.c',
    'libweston/color-representation.c',
    'libweston/color-noop.c',
    'libweston/color-operations.c',
    'libweston/color-profile-param-builder.c',
    'libweston/compositor.c',
    'libweston/content-protection.c',
    'libweston/commit-timing.c',
    'libweston/data-device.c',
    'libweston/drm-formats.c',
    'libweston/fifo.c',
    'libweston/id-number-allocator.c',
    'libweston/input.c',
    'libweston/linux-dmabuf.c',
    'libweston/linux-explicit-synchronization.c',
    'libweston/linux-sync-file.c',
    'libweston/log.c',
    'libweston/noop-renderer.c',
    'libweston/output-capture.c',
    'libweston/pixel-formats.c',
    'libweston/pixman-renderer.c',
    'libweston/plugin-registry.c',
    'libweston/surface-state.c',
    'libweston/timeline.c',
    'libweston/touch-calibration.c',
    'libweston/weston-log-wayland.c',
    'libweston/weston-log-file.c',
    'libweston/weston-log-flight-rec.c',
    'libweston/weston-log.c',
    'libweston/weston-direct-display.c',
    # ---- libweston-desktop（xdg-shell 服务端实现）----
    'libweston/desktop/libweston-desktop.c',
    'libweston/desktop/client.c',
    'libweston/desktop/seat.c',
    'libweston/desktop/surface.c',
    'libweston/desktop/xwayland.c',
    'libweston/desktop/xdg-shell.c',
    'libweston/desktop/xdg-shell-v6.c',
    # ---- shell 公共工具 ----
    'libweston/shell-utils/shell-utils.c',
    # ---- headless 后端 ----
    'libweston/backend-headless/headless.c',
    # ---- libshared：libweston 实际用到的子集 ----
    'shared/config-parser.c',
    'shared/option-parser.c',
    'shared/file-util.c',
    'shared/os-compatibility.c',
    'shared/process-util.c',
    'shared/hash.c',
    'shared/matrix.c',
    # ---- 预生成的协议代码（服务端）----
    '../wayland-protocols/gen/alpha-modifier-v1-protocol.c',
    '../wayland-protocols/gen/color-management-v1-protocol.c',
    '../wayland-protocols/gen/color-representation-v1-protocol.c',
    '../wayland-protocols/gen/commit-timing-v1-protocol.c',
    '../wayland-protocols/gen/fifo-v1-protocol.c',
    '../wayland-protocols/gen/linux-dmabuf-unstable-v1-protocol.c',
    '../wayland-protocols/gen/linux-explicit-synchronization-unstable-v1-protocol.c',
    '../wayland-protocols/gen/input-method-unstable-v1-protocol.c',
    '../wayland-protocols/gen/input-timestamps-unstable-v1-protocol.c',
    '../wayland-protocols/gen/presentation-time-protocol.c',
    '../wayland-protocols/gen/pointer-constraints-unstable-v1-protocol.c',
    '../wayland-protocols/gen/relative-pointer-unstable-v1-protocol.c',
    '../wayland-protocols/gen/single-pixel-buffer-v1-protocol.c',
    '../wayland-protocols/gen/tearing-control-v1-protocol.c',
    '../wayland-protocols/gen/tablet-unstable-v2-protocol.c',
    '../wayland-protocols/gen/text-cursor-position-protocol.c',
    '../wayland-protocols/gen/text-input-unstable-v1-protocol.c',
    '../wayland-protocols/gen/viewporter-protocol.c',
    '../wayland-protocols/gen/xdg-output-unstable-v1-protocol.c',
    '../wayland-protocols/gen/weston-content-protection-protocol.c',
    '../wayland-protocols/gen/weston-debug-protocol.c',
    '../wayland-protocols/gen/weston-direct-display-protocol.c',
    '../wayland-protocols/gen/weston-output-capture-protocol.c',
    '../wayland-protocols/gen/weston-touch-calibration-protocol.c',
)

add_includedirs(
    '.',                        # config.h
    './compat',                 # drm_fourcc.h / xf86drm.h / cairo.h / linux/ioctl.h
    './include',                # <libweston/libweston.h> 等对外头
    './libweston',              # 私有头（compositor.h/backend.h/pixman-renderer.h）
    './shared',                 # shared/helpers.h 等
    '../wayland-protocols/gen',  # 协议头
    public=True,
)
