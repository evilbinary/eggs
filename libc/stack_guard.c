/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include "stdio.h"

uintptr_t __stack_chk_guard=0xBADCAFE;

void __stack_chk_fail(){
  printf("stack oops\n");
  exit(-100);
}
