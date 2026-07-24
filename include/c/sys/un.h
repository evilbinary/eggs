#ifndef _SYS_UN_H
#define _SYS_UN_H

#include "stdint.h"

/* Unix domain socket address */
struct sockaddr_un {
    uint16_t sun_family;
    char sun_path[108];
};

#define SUN_PATH_MAX 108

#endif
