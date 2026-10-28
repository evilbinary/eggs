# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libinput 1.19.4（静态库，只出库；tools/test/doc 不编）。
#
# 不走 meson：config.h / libinput-version.h / libinput-git-version.h 手工给
# （做法与 libevdev、libwayland 一致，见同目录 config.h 顶部说明）。
#
# 两个外部依赖在本系统没有对应库，用 shim/ 下的最小实现顶上：
#   · libudev —— libinput 靠 udev 的 ID_INPUT_* 属性判设备类型；垫片用
#     libevdev 从【内核 evdev 节点】读真实能力位现算这些属性（等价 udev
#     的 input_id 内建），并扫 /dev/input 枚举。内核侧设备节点由
#     duck/modules/input 提供（/dev/input/eventN，ioctl 全集）。
#   · mtdev   —— 只有协议 A 老设备才用到，直通实现（事件原样进出）。
target("input")
set_kind("static")

add_packages(get_config('default_libc'))
# 事件解析靠 libevdev（eggs/libevdev，含 linux/input.h uapi 头）
add_deps("evdev")

add_files(
    'src/*.c'
)

add_includedirs(
    '.',              # config.h / libinput-version.h
    './src',          # libinput.h 等对外头
    './include',      # linux/input.h uapi 头
    public=True,
)
