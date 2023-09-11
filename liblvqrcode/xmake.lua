target("lvqrcode")
    set_kind("static")

    add_packages("musl")
    add_deps("lvgl")

    add_files(
        'lv_qrcode.c',
        'qrcodegen.c'
    )

    add_cflags('-DLV_LVGL_H_INCLUDE_SIMPLE=1 ')

    add_includedirs(
        '.'
    )