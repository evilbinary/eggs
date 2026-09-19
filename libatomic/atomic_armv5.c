/*******************************************************************
 * armv5 原子操作补充实现（给裸机上的 SDL2 / miniaudio 等库用）
 *
 * 背景：ARM926（armv5te）没有 LDREX/STREX/LDREXD 指令，GCC 会把
 * __atomic_xxx / __sync_xxx 这些内置函数编译成对同名库函数的调用；
 * 而 arm-none-eabi 的 libgcc 并不提供它们（仓库里 libgcc 的版本是
 * Linux 专用 __kernel_cmpxchg，裸机不可用），于是链接 SDL2
 * （SDL_atomic.c / SDL_spinlock.c）与 miniaudio 时会报 undefined reference。
 *
 * 本文件给**单核**场景提供最小实现：
 *   - 不做指令级原子化（硬件无此能力），只保证"读写该变量"这一动作完成；
 *   - 同步原语提供编译/内存屏障；
 *   - 本项目跑在单核上，且这些原子量（SDL 自旋锁、音频状态标志）在单线程
 *     应用里不存在竞争 ⇒ 语义等价。若以后要在多线程/多核下共享，需要改成
 *     经内核 SWI 的 CAS。
 *
 * 只在 armv5 下编译出符号：其它架构由 GCC 内联或 libgcc 正常提供，不重复定义。
 ********************************************************************/
#if defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5__) || defined(ARMV5) || \
    defined(ARM926)

typedef unsigned int u32a;
typedef int i32a;
typedef unsigned long long u64a;

#define SYNC_BARRIER() __asm__ __volatile__("" ::: "memory")

/* ================= 4 字节 ================= */

u32a __sync_fetch_and_add_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = old + val;
  return old;
}

u32a __sync_fetch_and_sub_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = old - val;
  return old;
}

u32a __sync_add_and_fetch_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  *p = *p + val;
  return *p;
}

u32a __sync_sub_and_fetch_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  *p = *p - val;
  return *p;
}

u32a __sync_fetch_and_or_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = old | val;
  return old;
}

u32a __sync_fetch_and_and_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = old & val;
  return old;
}

/* 返回旧值；相等才写入 */
u32a __sync_val_compare_and_swap_4(volatile void* ptr, u32a oldval, u32a newval) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  if (old == oldval) {
    *p = newval;
  }
  return old;
}

int __sync_bool_compare_and_swap_4(volatile void* ptr, u32a oldval, u32a newval) {
  volatile u32a* p = (volatile u32a*)ptr;
  if (*p == oldval) {
    *p = newval;
    return 1;
  }
  return 0;
}

u32a __sync_lock_test_and_set_4(volatile void* ptr, u32a val) {
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = val;
  return old;
}

void __sync_lock_release_4(volatile void* ptr) {
  *(volatile u32a*)ptr = 0;
  SYNC_BARRIER();
}

void __sync_synchronize(void) { SYNC_BARRIER(); }

/* ================= 8 字节 ================= */

u64a __sync_fetch_and_add_8(volatile void* ptr, u64a val) {
  volatile u64a* p = (volatile u64a*)ptr;
  u64a old = *p;
  *p = old + val;
  return old;
}

int __sync_bool_compare_and_swap_8(volatile void* ptr, u64a oldval, u64a newval) {
  volatile u64a* p = (volatile u64a*)ptr;
  if (*p == oldval) {
    *p = newval;
    return 1;
  }
  return 0;
}

u64a __sync_val_compare_and_swap_8(volatile void* ptr, u64a oldval, u64a newval) {
  volatile u64a* p = (volatile u64a*)ptr;
  u64a old = *p;
  if (old == oldval) {
    *p = newval;
  }
  return old;
}

/* ================= __atomic_xxx（GCC 内置的库形态） ================= */
/* memorder 参数在单核非竞争场景下无实际作用，这里忽略 */

u32a __atomic_load_4(const volatile void* ptr, int memorder) {
  (void)memorder;
  return *(const volatile u32a*)ptr;
}

void __atomic_store_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  *(volatile u32a*)ptr = val;
}

u64a __atomic_load_8(const volatile void* ptr, int memorder) {
  (void)memorder;
  return *(const volatile u64a*)ptr;
}

void __atomic_store_8(volatile void* ptr, u64a val, int memorder) {
  (void)memorder;
  *(volatile u64a*)ptr = val;
}

u32a __atomic_exchange_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  *p = val;
  return old;
}

u64a __atomic_exchange_8(volatile void* ptr, u64a val, int memorder) {
  (void)memorder;
  volatile u64a* p = (volatile u64a*)ptr;
  u64a old = *p;
  *p = val;
  return old;
}

u32a __atomic_fetch_add_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  return __sync_fetch_and_add_4(ptr, val);
}

u64a __atomic_fetch_add_8(volatile void* ptr, u64a val, int memorder) {
  (void)memorder;
  return __sync_fetch_and_add_8(ptr, val);
}

u32a __atomic_fetch_sub_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  return __sync_fetch_and_sub_4(ptr, val);
}

u32a __atomic_fetch_and_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  return __sync_fetch_and_and_4(ptr, val);
}

u32a __atomic_fetch_or_4(volatile void* ptr, u32a val, int memorder) {
  (void)memorder;
  return __sync_fetch_and_or_4(ptr, val);
}

/* 返回值类型用 _Bool 与 GCC 内置函数原型一致（避免 -Wbuiltin-declaration-mismatch） */
_Bool __atomic_compare_exchange_4(volatile void* ptr, void* expected,
                                  u32a desired, _Bool weak,
                                  int success_memorder, int failure_memorder) {
  (void)weak;
  (void)success_memorder;
  (void)failure_memorder;
  volatile u32a* p = (volatile u32a*)ptr;
  u32a old = *p;
  u32a exp = *(u32a*)expected;
  if (old == exp) {
    *p = desired;
    return 1;
  }
  *(u32a*)expected = old;
  return 0;
}

_Bool __atomic_compare_exchange_8(volatile void* ptr, void* expected,
                                  u64a desired, _Bool weak,
                                  int success_memorder, int failure_memorder) {
  (void)weak;
  (void)success_memorder;
  (void)failure_memorder;
  volatile u64a* p = (volatile u64a*)ptr;
  u64a old = *p;
  u64a exp = *(u64a*)expected;
  if (old == exp) {
    *p = desired;
    return 1;
  }
  *(u64a*)expected = old;
  return 0;
}

#endif /* armv5 */
