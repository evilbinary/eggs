/* pixman 0.42.2 手工配置头（不走 configure/meson）。
 *
 * 与 libwayland / libevdev / libinput 的做法一致：只声明构建期探测会生成的
 * 宏，构建系统（ya.py）把本目录加进 include path，pixman 源码里的
 * #include <config.h> 即指向这里。
 *
 * 取舍：
 *   - 不定义任何 USE_ARM_NEON / USE_ARM_SIMD / USE_SSE2 之类宏，只编通用 C
 *     路径（无 NEON 汇编）。当前场景是几百×几百像素的合成，C 版足够，且
 *     aarch64 汇编文件在 -elf 工具链下要额外的 .S 规则，先不引入。
 *   - HAVE_PTHREADS：musl 提供 pthread，打开后 pixman 用线程本地变量缓存
 *     实现选择（pixman-implementation.c）。
 */
#ifndef PIXMAN_CONFIG_H
#define PIXMAN_CONFIG_H

#define HAVE_PTHREADS 1

/* __builtin_clz 由 gcc/clang 提供 */
#define HAVE_BUILTIN_CLZ 1

/* long 在 aarch64 LP64 下是 8 字节 */
#define SIZEOF_LONG 8

/* 【必须不定义】pixman 用这个宏决定 implementation 表的初始化方式：
 *   · 定义了  ⇒ 只靠 __attribute__((constructor)) 的 pixman_constructor()
 *                填 global_implementation，get_implementation() 里没有懒初始化；
 *   · 没定义  ⇒ get_implementation() 里 "if (!global_implementation) choose()"。
 * 实测（weston-desktop-shell，pixman 静态链进可执行文件）：构造函数没有
 * 被执行 ⇒ global_implementation 恒 NULL ⇒ get_implementation() 返回 NULL
 * ⇒ _pixman_implementation_fill/composite 遍历空链直接返回 FALSE
 * ⇒ **所有 pixman 绘制静默变成 no-op**（pixman_fill 返回 0、
 * pixman_image_fill_boxes 仍然返回 TRUE，所以上层完全察觉不到）。
 * 症状：cairo 的 paint/fill 一个像素都写不进去、`status` 却一直是 0；
 * 而 libweston.so 里那份 pixman 因为共享库的构造函数由 ldso 执行，是正常的
 * （compositor 能画背景，客户端画不出来）。
 * 这里改成懒初始化后不依赖任何构造函数，两种链接方式都对。 */
/* #define TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR 1 */

/* TLS 关键字：pixman 用它做实现缓存；不能同时定义 PIXMAN_NO_TLS */
#define TLS __thread

#define PACKAGE "pixman"

#endif /* PIXMAN_CONFIG_H */
