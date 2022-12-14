#include "sys/mman.h"
#include "stdarg.h"

void *mremap(void *old_addr, size_t old_len, size_t new_len, int flags, ...) {
  va_list ap;
  void *new_addr = 0;

//   if (new_len >= PTRDIFF_MAX) {
//     errno = ENOMEM;
//     return MAP_FAILED;
//   }

  if (flags & MREMAP_FIXED) {
    // __vm_wait();
    va_start(ap, flags);
    new_addr = va_arg(ap, void *);
    va_end(ap);
  }

  return ya_mremap(old_addr, old_len, new_len, flags, new_addr);
}