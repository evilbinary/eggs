add_requires('ffmpeg')

package("ffmpeg")

    add_deps("automake") --"libtool", "pkg-config"

    set_sourcedir(os.scriptdir())

    add_includedirs(
        "include",
        ".",
        {public = true}
    )
    add_cflags(' -Wno-error=pointer-sign -DSYSCALL_NO_TLS ',{public = true})

    on_install(function (package)

        toolchains=package:configs().toolchains
        compile=toolchains..'-'

        cflags=package:build_getenv("cflags")
        ldflags=package:build_getenv("cflags")
        cc= package:build_getenv("cc")
        ar= package:build_getenv("ar")
        ranlib= package:build_getenv("ranlib")

        print('cc=>',cc)
        print('ar=>',ar)
        print('ranlib=>',ranlib)


        configs = {
        '--cc='..cc,
        '--ar='..ar,
        '--ranlib='..ranlib,
        '--enable-cross-compile',
        '--arch='..arch,
        '--target-os=linux',
       -- '--cross-prefix='+env['CC_PREFIX'],
        '--extra-cflags='..table.concat(cflags, " ") ,
        '--extra-ldflags='..table.concat(ldflags, " "),
        '--host-extralibs=',
        '--enable-gpl',
        '--enable-nonfree',
        '--enable-static',
        '--disable-programs',
        '--disable-stripping',
        '--disable-doc',
        '--disable-pthreads',
        '--disable-shared',
        '--disable-htmlpages',
        '--disable-manpages',
        '--disable-podpages',
        '--disable-txtpages',

       -- '--disable-encoders ',
       -- '--disable-decoders ',
       -- '--enable-decoder=aac',
       -- '--enable-decoder=aac_at',
       -- '--enable-decoder=h264',
       -- '--enable-decoder=h264_mediacodec',
       -- '--disable-demuxers ',
       -- '--enable-demuxer=aac',
       -- '--enable-demuxer=h264',
        '--disable-vfp',
       -- '--disable-neon',
        '--disable-inline-asm',


        '--disable-dct',
        '--disable-asm',
        '--disable-network'

       -- '--disable-yasm',
       -- '--disable-yasm',
       -- '--disable-stripping',
       --
       -- '--disable-encoders',
       -- '--disable-decoders',
       -- '--enable-decoder=aac',
       -- '--enable-decoder=aac_at',
       -- '--enable-decoder=h264',
       -- '--enable-decoder=h264_mediacodec',
       -- '--disable-demuxers ',
       -- '--enable-demuxer=aac',
       -- '--enable-demuxer=h264',
        }

        import("package.tools.autoconf").install(package, configs)        
    end)
