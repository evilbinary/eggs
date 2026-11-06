# coding:utf-8
target("gpuclient")
set_kind("static")
add_cflags('-fPIC')
add_packages(get_config('default_libc'))
add_files('src/gpu_client.c')
add_includedirs('.', './include', public=True)
add_deps('foot')
