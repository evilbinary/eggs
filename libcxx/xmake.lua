target("cxx")
    set_kind("static")

    add_packages("musl")

    add_files(
        'src/*.cpp',
        'src/filesystem/*.cpp'
    )
    
    add_includedirs(
    '.',
    './include',
    '../../duck/libs/include',
    './include/experimental'
    , {public = true}
    )
    add_cxxflags(
        '-ffreestanding -fpic',
                '-fpermissive',
                 '-fno-use-cxa-atexit -fno-threadsafe-statics -D_LIBCPP_HAS_MUSL_LIBC -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION    -D_LIBCPP_BUILDING_LIBRARY -D_POSIX_C_SOURCE  -D_GNU_SOURCE -D_LIBCPP_HAS_NO_MONOTONIC_CLOCK  -D_LIBCPP_HAS_NO_THREADS -D_LIBCXXABI_HAS_NO_THREADS '
    , {public = true})
    add_defines(def_arch)
    add_defines(def_arch_type)
