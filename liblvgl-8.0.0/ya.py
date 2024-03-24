target("lvgl-8.0.0")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("gui")

add_files(
    # 'lv_conf.h',
    'port/lv_port_disp.c',
    # "port/lv_port_fs.c',",
    'port/mouse_cursor_icon.c',
    'port/lv_port_indev.c'
)

add_files(
    'src/core/*.c',
    'src/draw/*.c',
    'src/font/*.c',
    'src/gpu/*.c',
    'src/hal/*.c',
    'src/misc/*.c',
    'src/widgets/*.c',
    'src/extra/*.c',
    'src/extra/**/*.c',
    'src/extra/**/**/*.c'
)

add_cflags('-DLV_CONF_INCLUDE_SIMPLE ')


add_includedirs(
    '.',
    '../include',
    public = true
)