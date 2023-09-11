target("webp")
    set_kind("static")

    add_packages("musl")

    add_files(
        "src/dec/*.c",
        'src/demux/*.c',
        'src/dsp/*.c',
        'src/enc/*.c',
        'src/mux/*.c',
        'src/utils/*.c'
    )
    
    add_includedirs(
    '.',
    './src',
    {public = true}
    )