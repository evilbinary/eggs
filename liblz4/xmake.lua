target("lz4")
    set_kind("static")

    add_packages("musl")

    add_files(
     'lib/lz4.c',
      'lib/lz4frame.c',
      'lib/lz4hc.c',
      'lib/xxhash.c'
    )
    
    add_includedirs(
        '.',
        '../include',
        'lib/',
        {public = true}
    )