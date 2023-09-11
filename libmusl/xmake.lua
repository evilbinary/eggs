add_requires('musl')

package("musl")

    add_deps("automake") --"libtool", "pkg-config"

    set_sourcedir(os.scriptdir())

    add_includedirs(
        "include",
        {public = true}
    )
    add_cflags(' -Wno-error=pointer-sign -DSYSCALL_NO_TLS ',{public = true})

    on_install(function (package)
        toolchains=package:configs().toolchains
        compile=toolchains..'-'

        cflags=package:build_getenv("cflags")

        configs = {
            'CROSS_COMPILE='..compile,
            'CFLAGS=-Wno-error=pointer-sign -DSYSCALL_NO_TLS '.. table.concat(cflags, " ") ,
            '--disable-optimize',
            '--enable-debug=yes',
            -- '--enable-shared=yes',
            '--disable-shared',
            '--enable-static=yes'
        }

        import("package.tools.autoconf").install(package, configs)        
    end)
