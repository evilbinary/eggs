target("sdl2-image")
    set_kind("static")

    add_packages("musl")
    add_deps("sdl2")

    add_files(
        '*.c'
    )
    
    add_cflags(' -D__YIYIYA__ ')

    add_includedirs(
    '.',
    './include',
    '../libgui/',
     {public = true}
    )