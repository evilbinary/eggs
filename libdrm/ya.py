# coding:utf-8
target("drm")
set_kind("static")

add_cflags('-fPIC')

add_packages(get_config('default_libc'))

add_files(
    'src/drm.c',
    'src/v3d.c',
    'src/mali.c',
    'src/mesa_compat.c'
)

add_includedirs(
'.',
'./include',
public = true
)

add_deps('foot')
