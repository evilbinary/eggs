# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("jpeg")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'jcapimin.c', 'jcapistd.c', 'jccoefct.c', 'jccolor.c', 'jcdctmgr.c', 'jchuff.c',
    'jcinit.c', 'jcmainct.c', 'jcmarker.c', 'jcmaster.c', 'jcomapi.c', 'jcparam.c',
    'jcphuff.c', 'jcprepct.c', 'jcsample.c', 'jctrans.c', 'jdapimin.c', 'jdapistd.c', 'jdatadst.c', 'jdatasrc.c', 'jdcoefct.c', 'jdcolor.c', 'jddctmgr.c', 'jdhuff.c',
        'jdinput.c', 'jdmainct.c', 'jdmarker.c', 'jdmaster.c', 'jdmerge.c', 'jdphuff.c', 'jdpostct.c', 'jdsample.c', 'jdtrans.c', 'jerror.c', 'jfdctflt.c', 'jfdctfst.c', 'jfdctint.c', 'jidctflt.c', 'jidctfst.c', 'jidctint.c', 'jidctred.c', 'jquant1.c', 'jquant2.c', 'jutils.c', 'jmemmgr.c', 'jmemansi.c'
)

add_includedirs(
    '.',
    '../include',
    public = true
)
