target("lua")
    set_kind("static")

    add_packages("musl")

    add_files(
        "src/*.c"
    )
    
    add_includedirs(
        '.',
        '../include',
        './src',
        {public = true}
    )