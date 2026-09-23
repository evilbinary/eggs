#include <stdint.h>
#include <elf.h>
#include "pthread_impl.h"
#include "libc.h"

#define HWCAP_TLS (1 << 15)

extern hidden const unsigned char
	__a_barrier_oldkuser[], __a_barrier_v6[], __a_barrier_v7[],
	__a_cas_v6[], __a_cas_v7[],
	__a_gettp_cp15[];

#define __a_barrier_kuser 0xffff0fa0
#define __a_barrier_oldkuser (uintptr_t)__a_barrier_oldkuser
#define __a_barrier_v6 (uintptr_t)__a_barrier_v6
#define __a_barrier_v7 (uintptr_t)__a_barrier_v7

#define __a_cas_kuser 0xffff0fc0
#define __a_cas_v6 (uintptr_t)__a_cas_v6
#define __a_cas_v7 (uintptr_t)__a_cas_v7

#define __a_gettp_kuser 0xffff0fe0
#define __a_gettp_cp15 (uintptr_t)__a_gettp_cp15

extern hidden uintptr_t __a_barrier_ptr, __a_cas_ptr, __a_gettp_ptr;

/* 【修复】ARMv5（ARM926）没有 LDREX/STREX，用户态本无法实现 CAS（上游 musl
 * 靠内核 kuser helper 0xffff0fc0）。本 fork 之前把它换成了 "return 0" 的桩：
 * 而 a_cas 的契约是"返回 0 = 比较交换成功且内存已写入" —— 于是 ARMv5 上
 * musl 的全部原子操作（mallocng 维护 avail_mask/freed_mask、锁、计数）都成了
 * 无声 no-op，堆元数据必然损坏；raspi2(armv7) 正常是因为这段
 * #if __ARM_ARCH < 7 根本不参与编译。
 * 这里用 swp（原子读-写，ARMv2+ 即有）实现真 CAS：
 *   成功：swp 换入新值，返回 0；
 *   失败：swp 把旧值换回去（a_cas 包装层会自己重读 *p 并返回实际值，
 *         所以必须真正还原内存，否则包装层会在 for(;;) 里死转）。
 * 局限：swp 原子，但"比较+还原"两步之间若被抢占仍可能丢更新 —— 纯 UP 且
 * 用户态互斥依赖 musl 自身锁时安全；将来若上抢占式多线程用户程序，应改成
 * 内核态 CAS（trap 进内核关中断完成）。 */
__attribute__((naked, noinline)) static int cpu_cmpxchg(volatile void* ptr, int old_value, int new_value) {
	/* 调用方（atomic_arch.h 的 a_cas）约定：r0=old_value, r1=new_value,
	 * r2=ptr；只允许破坏 r0/r3/ip/lr/flags，r1/r2 必须原样保留。 */
	__asm__ __volatile__(
		"swp   r3, r1, [r2]   \n" /* r3 = 旧值; [r2] = 新值   */
		"cmp   r3, r0         \n" /* 旧值 == 期望值 ?         */
		"moveq r0, #0         \n" /* 成功                     */
		"beq   1f             \n"
		"swp   ip, r3, [r2]   \n" /* 失败：把旧值换回去       */
		"mov   r0, #1         \n"
		"1: bx  lr            \n"
	);
}

static inline int barrier(){
	 __asm__ __volatile__("" : : : "memory");
	 return 1;
}


int __set_thread_area(void *p)
{
#if !__ARM_ARCH_7A__ && !__ARM_ARCH_7R__ && __ARM_ARCH < 7
	if (__hwcap & HWCAP_TLS) {
		size_t *aux;
		__a_cas_ptr = __a_cas_v7;
		__a_barrier_ptr = __a_barrier_v7;
		for (aux=libc.auxv; *aux; aux+=2) {
			if (*aux != AT_PLATFORM) continue;
			const char *s = (void *)aux[1];
			if (s[0]!='v' || s[1]!='6' || s[2]-'0'<10u) break;
			__a_cas_ptr = __a_cas_v6;
			__a_barrier_ptr = __a_barrier_v6;
			break;
		}
	} else {
		/* ARMv5：走内核 kuser helper（由 duck/modules/musl 在 0xffff0000
		 * 提供自包含指令页）。ver 读自 0xffff0ffc。 */
		int ver = *(int *)0xffff0ffc;
		__a_gettp_ptr = __a_gettp_kuser;
		__a_cas_ptr = __a_cas_kuser;
		__a_barrier_ptr = __a_barrier_kuser;
		if (ver < 2) a_crash();
		if (ver < 3) __a_barrier_ptr = __a_barrier_oldkuser;
	}
#endif
	return __syscall(0xf0005, p);
}
