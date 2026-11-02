# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
arch_type=get_arch_type()

default_libc=get_config('default_libc')

if default_libc:
    includes("./lib"+default_libc+"/ya.py")

if has_config("cpp-apps"):
    cpp_libs=['libcxx', 'libcxxabi']
    for v in cpp_libs:
        includes("./"+v+"/ya.py")



if has_config("app"):
    c_libs=[
        'libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
        'liblvgl', 'liblvgl-8.0.0', 'libuuid',
        'liblvqrcode', 'liblz4', 'libfreetype-2.14.3',
        'libwebp', 'libsdl2', 'libsdl2-ttf',
        'libsdl2-image', 'liblua',
        'libcmocka', 'libffmpeg',
        'libsdl', 'libsdl-ttf', 'libsdl-image',
        'libfoot',
        'libdrm',
        # OpenSSL 的 libcrypto（sshd 用；只出 crypto，不要 ssl）
        'libopenssl',
        # libxcrypt 的 libcrypt（sshd 密码认证用 crypt(3)；musl 不自带）
        'libxcrypt',
        # 'libiconv'
        'libncurses',
        'libsdl-gfx',
        'libquickjs',
        'libyui',
        'libcjson',
        # wayland 核心库（client/server/shm/cursor，协议代码预生成）
        # 依赖 libffi（libwayland 用 ffi_call 调用户实现的协议函数）
        'libffi', 'libevdev', 'libwayland',
        # pixman：libweston 的 pixman 渲染器依赖
        'libpixman', 'libxkbcommon', 'weston',
        # libinput 及其两个依赖库：libmtdev（协议 A→B 转换）、
        # libudev（libudev-zero 实现，设备枚举/属性）。
        'libmtdev', 'libudev', 'libinput',
        # armv5（无 LDREX）专用：补 libgcc 缺失的 __sync_xxx/__atomic_xxx
        'libatomic',
        'libxml2',
        'libcairo',
        # Mesa 的 v3d 子集（ya 构建，不用 meson）：broadcom 编译器
        # + NIR + 驱动，配套 eggs/libdrm 替身
        'libmesa',


    ]

    if default_libc=='c':
        c_libs=[
            'libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
            'liblvgl', 'liblvgl-8.0.0', 'libuuid',
            'liblvqrcode', 'liblz4', 'libfreetype-2.14.3',
            'libwebp', 'libsdl2', 'libsdl2-ttf',
            'libsdl2-image', 'liblua',
            'libcmocka', 'libffmpeg',
            'libsdl', 'libsdl-ttf', 'libsdl-image',
            'libfoot',
            # 'libiconv'
            # 'libncurses',
            'libsdl-gfx',
            'libquickjs',
            'libyui',
            'libcjson',
            # armv5（无 LDREX）专用：补 libgcc 缺失的 __sync_xxx/__atomic_xxx
            'libatomic'
        ]
    

    if arch_type in['armv7-a','x86']:
        c_libs+=['libchez',]

    for v in c_libs:
        includes("./"+v+"/ya.py")


# wayland 基础层只放 wayland 自身需要的库。cairo/pango 这类图形库不在这里：
# 挂在 wayland 下面，别的 app 想用 cairo 就取不到（被 wayland 选项饿死）。
if has_config("wayland"):
    wayland_libs=['libwayland']
    for v in wayland_libs:
        includes("./"+v+"/ya.py")

