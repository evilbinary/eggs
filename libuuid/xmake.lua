target("uuid")
    set_kind("static")

    add_packages("musl")

    add_files(
        'uuid/compare.c',
        'uuid/gen_uuid.c',
        'uuid/isnull.c',
        'uuid/parse.c',
        'uuid/unpack.c',
        'uuid/clear.c',
        'uuid/copy.c',
        'uuid/pack.c',
        'uuid/unparse.c',
        'uuid/uuid_time.c'
    )
    
    add_includedirs(
    '.',
    './uuid',
    {public = true}
    )