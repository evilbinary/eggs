target("c")
    set_kind("static")

    add_packages("musl")

    add_files(
        'string.c',
        'malloc.c',
        'vsprintf.c',
        'stdio.c',
        'stdlib.c',
        'syscall.c',
        'errno.c',
        'ctype.c',
        'support.c',
        'unistd.c',
        'fcntl.c',
        'yiyiya.c',
        'time.c',
        'setjmp.c',
        'signal.c',
        'timeb.c',
        'strings.c',
        'gettimeofday.c',
        'math.c',
        'getopt.c',
        'ioctl.c',
        'dirent.c',
        'assert.c',
        'locale.c',
        'vsscanf.c',
        'strntoumax.c',
        'vsnprintf.c',
        'vfscanf.c',
        'dlfcn.c',
        'password.c',
        'fenv.c',
        'alloc.c',
        'yiyiya-alloc.c',
        'stack_guard.c',
        'mman.c',
        'math/*.c',
        'environ/*.c',
        'sys/*.c',
        'crt/*.c'
    )

    arch_type= get_arch_type(get_config("arch") )
    
    if arch_type =='arm':
        add_files(
            'arm/longjmp.S',
            'arm/setjmp.S'
        )
    elif arch_type=='x86':
        add_files(
            'i386/longjmp.s',
            'i386/setjmp.s'
        )

    add_includedirs(
        '.',
        '../include'
    )