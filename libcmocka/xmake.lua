target("cmocka")
    set_kind("static")

    add_packages("musl")

    add_files(
     'src/*.c'
    )

    add_cflags('-DHAVE_SIGNAL_H')
    
    add_includedirs(
        '.',
        './include'
    )