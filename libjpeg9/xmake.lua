target("jpeg9")
    set_kind("static")

    add_packages("musl")

    add_files(
      '*.c'
    )
    
    add_includedirs(
        '.',
        '../include'
    )