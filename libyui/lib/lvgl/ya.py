# coding:utf-8

import os

_LVGL_DIR = os.path.dirname(os.path.abspath(__file__))


def _extra_widget_sources():
    """List extra widget .c files (ya glob does not match nested paths on Windows)."""
    root = os.path.join(_LVGL_DIR, 'src', 'extra', 'widgets')
    sources = []
    if not os.path.isdir(root):
        return sources
    for sub in sorted(os.listdir(root)):
        subdir = os.path.join(root, sub)
        if not os.path.isdir(subdir):
            continue
        for name in sorted(os.listdir(subdir)):
            if name.endswith('.c'):
                sources.append('src/extra/widgets/%s/%s' % (sub, name))
    return sources


# layouts/themes + lv_extra.c — required when LV_USE_FLEX/GRID/THEME_DEFAULT are enabled
_EXTRA_CORE = [
    'src/extra/lv_extra.c',
    'src/extra/layouts/flex/lv_flex.c',
    'src/extra/layouts/grid/lv_grid.c',
    'src/extra/themes/default/lv_theme_default.c',
    'src/extra/themes/basic/lv_theme_basic.c',
    'src/extra/themes/mono/lv_theme_mono.c',
]

target("lvgl")
set_kind("static")

add_flags()  # libc 头文件路径（musl 等），设备平台没有系统头文件可用

add_files(
    'src/core/*.c',
    'src/draw/*.c',
    'src/draw/sw/*.c',
    'src/font/*.c',
    'src/hal/*.c',
    'src/misc/*.c',
    'src/widgets/*.c',
)
# extra widgets + layouts/themes 与 core 放同一库：lv_init() 引用 lv_extra_init，
# 而 extra 又引用 core，跨库会形成静态库循环依赖（ld 单遍无法解析）。
add_files(*_extra_widget_sources())
add_files(*_EXTRA_CORE)

add_cflags('-DLV_CONF_INCLUDE_SIMPLE')

add_includedirs('.', './src', public=True)

if get_plat() == "yiyiya" or not is_host_plat():
    # 设备平台（raspi2/3/5、v3s 等）用 YiYiYa 原生 framebuffer 端口
    add_files('port_yiyiya/*.c')
    add_cflags('-DYUI_LVGL_PORT_YIYIYA', public=True)
    # screen.h / event.h 在 libgui 里（ymake 的 include 相对路径是按构建目录
    # 拼接的，这里必须用绝对路径，直接走 -I cflags 最稳妥）
    _yiyiya_root = os.path.normpath(os.path.join(_LVGL_DIR, '..', '..', '..', '..'))
    _libgui_dir = os.path.join(_yiyiya_root, 'eggs', 'libgui')
    _duck_libs = os.path.join(_yiyiya_root, 'duck', 'libs', 'include')
    _duck_dir = os.path.join(_yiyiya_root, 'duck')
    add_cflags('-I' + _libgui_dir, '-I' + _duck_libs, '-I' + _duck_dir,
               public=True)
elif get_plat() in ("esp32", "stm32"):
    # 嵌入式平台用 YUI 原生后端，不依赖 LVGL，不编译任何 port
    pass
else:
    add_files('port_sdl/*.c')
    add_cflags('-DYUI_LVGL_PORT_SDL', public=True)

# 兼容旧依赖名：extra widgets 已并入 lvgl 单库，这里只保留空壳让
# add_deps("lvgl_extra") 仍能解析到 lvgl。
target("lvgl_extra")
set_kind("static")
add_deps("lvgl")
add_cflags('-DLV_CONF_INCLUDE_SIMPLE')
add_includedirs('.', './src', public=True)
