target("freetype")
    set_kind("static")

    add_packages("musl")

    add_files(
        'src/**/*.c'
    )

    add_cflags('-DSDL_IMAGE_USE_COMMON_BACKEND',
		'-DLOAD_BMP -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM',
		'-DLOAD_SVG -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV',
		 '-DLOAD_PNG -DLOAD_WEBP',
        '-DLOAD_JPG')
    
    add_includedirs(
        '.',
        './include',
        {public = true}
    )