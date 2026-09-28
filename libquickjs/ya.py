target("quickjs")
set_type("lib")
set_kind("static")

add_packages(get_config('default_libc'))


add_files(
    'quickjs.c',
    'libregexp.c',
    'libunicode.c',
    'cutils.c',
    'quickjs-libc.c',
    'libbf.c'
) 

# YiYiYa: 控制台 UI 的逻辑是 JS 跑的，quickjs 在热路径上；原来没有 -O
# （默认 -O0），这里开 -O2，同时关掉严格别名以防老代码出问题。
add_cflags(' -UCONFIG_PRINTF_RNDN -D_GNU_SOURCE -DUSE_FILE32API  -I. -DCONFIG_BIGNUM -O2 -fno-strict-aliasing')

add_includedirs(
    '.',
    '../include',
    public = true
)
