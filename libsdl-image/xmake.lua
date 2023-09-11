target("sdl-image")
    set_kind("static")

    add_packages("musl")
    add_deps("sdl","webp")

    add_files(
        '*.c'
    )
    
    add_cflags('-DSDL_IMAGE_USE_COMMON_BACKEND',
		'-DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM',
		'-DLOAD_SVG -DLOAD_TGA  -DLOAD_XPM -DLOAD_XV',
		'-DLOAD_PNG',
        '-DFAST_IS_JPEG -DLOAD_WEBP -DLOAD_BMP -DLOAD_XCF -DLOAD_JPG'
        )

    add_includedirs(
    '.',
    '../libgui/',
     {public = true}
    )