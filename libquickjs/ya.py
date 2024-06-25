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

add_cflags(' -UCONFIG_PRINTF_RNDN -D_GNU_SOURCE -DUSE_FILE32API  -I. -DCONFIG_BIGNUM')
