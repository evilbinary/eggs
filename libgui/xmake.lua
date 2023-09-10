target("gui")
    set_kind("static")

    add_packages("musl")
    add_deps("jpeg","png","algorithm")

    add_files(
        'screen.c',
        'event.c'
    )
    
    add_includedirs(
        '.',
        '../include',

        {public = true}
    )

target("image")
    set_kind("static")

    add_packages("musl")
     add_deps("jpeg","png","algorithm")

    add_files(
       'image.c',
        'jpeg_decoder.c',
        'png_decoder.c'
    )
    
    add_includedirs(
        '.',
        {public = true}
    )
