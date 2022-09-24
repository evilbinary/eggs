static inline uintptr_t __get_tp()
{
	uintptr_t tp;
	//todo
	// __asm__ ("movl %%gs:0,%0" : "=r" (tp) );
	tp= __syscall(SYS_getpid);
	return tp;
}

#define MC_PC gregs[REG_EIP]
