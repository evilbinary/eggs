# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# weston 16.0.1 的 libweston（共享库 libweston.so：headless + pixman 渲染器）。
#
# 不走 meson：config.h / git-version.h / include/libweston/version.h 手工给，
# 协议代码用 eggs/wayland-protocols/gen/ 里预生成的（见其 gen.sh）。
#
# 内容 = 上游 libweston/meson.build 的 srcs_libweston + libweston-desktop +
# shell-utils + libshared 的构建期必需子集。
#
# 为什么是共享库：本系统现在有真正的动态加载（musl ldso），weston 本体按上游
# 架构运行——可执行文件 NEEDED libweston.so，shell/backend 模块是独立 .so，
# 由 dlopen 加载并从 libweston.so 解析符号。
#
# 裁掉的东西（各自拖大依赖）：
#   renderer-vulkan、backend-{drm,wayland,x11,pipewire,rdp,vnc}、
#   color-lcms（lcms2）、clients/（cairo/pango）、tests。
# renderer-gl 不编进 libweston：按上游做法是独立的 gl-renderer.so 模块
# （target 见 app/wayland/ya.py），这里只负责按需生成它的 GLSL 头文件。
#
# 依赖：wayland（client+server）、pixman、xkbcommon。

# ---- gl-renderer 的 GLSL 头 ----
# meson 里 vertex-shader.h / fragment-shader.h 是 custom_target；这里在
# ya 配置阶段按需生成：文件缺失、大小为 0，或 .glsl 比头文件新时才跑
# 上游自带的 tools/xxd.py。生成形式是
#   static const char vertex_shader[] = { ... };
# 正好是 libweston/renderer-gl/gl-shaders.c include 的符号。
import os
import subprocess

_BASE = os.path.dirname(os.path.abspath(__file__))
_GLDIR = os.path.join(_BASE, 'libweston/renderer-gl')


def _ensure_shader_header(src_name, hdr_name, var_name):
    src = os.path.join(_GLDIR, src_name)
    hdr = os.path.join(_GLDIR, hdr_name)
    if (os.path.exists(hdr) and os.path.getsize(hdr) > 0 and
            os.path.getmtime(hdr) >= os.path.getmtime(src)):
        return
    subprocess.run(['python3', os.path.join(_BASE, 'tools/xxd.py'),
                    src, hdr, '-n', var_name], check=True)
    print('weston: generated %s' % os.path.relpath(hdr, _BASE))


def ensure_gl_shader_headers():
    _ensure_shader_header('vertex.glsl', 'vertex-shader.h', 'vertex_shader')
    _ensure_shader_header('fragment.glsl', 'fragment-shader.h',
                          'fragment_shader')


ensure_gl_shader_headers()

target("weston")
set_kind("shared")
# 桌面启动性能：即使 debug 构建也给 libweston 开 -O2（-g 仍保留）。
add_cflags('-O2')
# 共享库必须按位置无关编（模块和可执行文件都要通过 GOT/PLT 引用它）
add_cflags('-fPIC')

add_packages(get_config('default_libc'))

# weston 源码里到处是 #include "config.h"，但整个仓库的 include path 上已经有
# duck/kernel/config.h（内核配置），它会先命中，把 weston 的构建配置顶掉。
# 因此自己的配置叫 weston-config.h，并用 -include 强制先吃进来。
add_cflags('-include eggs/weston/weston-config.h')

# 这三个是静态库（也都按 -fPIC 编，见各自的 ya.py），随 libweston.so 一起链进去。
add_deps("wayland", "pixman", "xkbcommon")
# 只为 <linux/input.h>（compositor.c / input.c 用 input_event 等结构）；
# libevdev 自身不会被链接进来（没有符号被引用）。
add_deps("evdev")

add_files(
    # ---- 缺失依赖的替身（libdrm 名字查询、cairo 窗口边框）----
    'compat/drm-stubs.c',
    'compat/frame-stubs.c',
    'compat/libdisplay-info-stub.c',
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
    # 注意：headless 后端（libweston/backend-headless/headless.c）不在这里，
    # 它按上游的做法编成独立模块 headless-backend.so，运行期由 dlopen 加载。
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
    # libweston-desktop 的 xdg-shell 服务端实现依赖这两个接口定义
    '../wayland-protocols/gen/xdg-shell-protocol.c',
    '../wayland-protocols/gen/xdg-shell-unstable-v6-protocol.c',
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
# ENABLE_EGL：pixel-formats.c 需要 EGL/GLES 头。只给 libweston 自己用，
# 不 public 传播 —— 否则会进 weston 前端的 include 路径，和 libinput 的
# linux/input.h 垫片抢顺序（实测前端 KEY_S/EV_KEY 变成未定义）。
add_includedirs('../../eggs/libmesa/include')
