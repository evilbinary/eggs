# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libudev 静态库（libudev-zero 1.0.4 实现，API/ABI 兼容 libudev）。
#
# 为什么用它而不是 systemd 的 libudev：systemd 版只是 sd-device 的薄包装，
# 绑定 hashmap/siphash/strv/path-util/netlink/udev 数据库等一大堆内部设施，
# 且运行时必须有 sysfs + netlink + /run/udev 数据。libudev-zero 是同一个
# API 的独立实现（纯 C99、零运行时依赖、可静态链接），适合本系统。
#
# 数据来源：/sys 下按 Linux 布局读属性、/sys/dev/char 枚举设备节点。
# 内核侧 sysfs 由 posix 层提供（见 sysfn_fs.c 的 sysfs 垫片）。
target("udev")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'udev.c',
    'udev_device.c',
    'udev_enumerate.c',
    'udev_list.c',
    'udev_monitor.c',
)

# -D_XOPEN_SOURCE=700：上游 meson 就这么加（strdup/realpath 等 XSI 接口）
# -DUSB_IDS_PATH：usb.ids 数据库路径，只用于把 ID_VENDOR_ID 翻成厂商名，
#                 文件不存在时只是少几个属性，不影响设备枚举。
add_cflags(
    '-D_XOPEN_SOURCE=700',
    '-DUSB_IDS_PATH="/usr/share/hwdata/usb.ids"',
)

add_includedirs(
    '.',           # libudev.h（转接 udev.h）
    './include',   # linux/{input,netlink,types}.h uapi 头（musl 不提供）
    public=True,
)
