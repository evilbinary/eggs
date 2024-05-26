target("quickjs")
set_type("lib")

add_files(
    'quickjs.c',
    'libregexp.c',
    'libunicode.c',
    'cutils.c',
    'quickjs-libc.c',
    'libbf.c'
) 

add_cflags(' -UCONFIG_PRINTF_RNDN -D_GNU_SOURCE -DUSE_FILE32API  -I. -DCONFIG_BIGNUM')
