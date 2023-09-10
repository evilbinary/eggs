target("gaga")
    set_kind("static")

    add_packages("musl")

    add_files(
        "*.c"
    )
    
    add_includedirs(
        '.',
        '../include',
        {public = true}
    )