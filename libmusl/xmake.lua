-- add_rules("mode.debug", "mode.release","arch")
-- add_repositories("local-repo build")
-- add_packages("libmusl")
add_requires('musl')

package("musl")

    add_deps("automake") --"libtool", "pkg-config"

    set_sourcedir(os.scriptdir())

    add_includedirs(
        "include",
        "obj/include/",
        "arch/generic/",
        "arch/generic/bits"
    )

    on_install(function (package)


        toolchains=package:configs().toolchains
        compile=toolchains..'-'
        -- compile=package:tool('cc')

      
        cflags=package:build_getenv("cflags")
        configs = {
            configure = "path/to/package/configure",
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
