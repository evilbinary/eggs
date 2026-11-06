/* mesa_compat.c - Mesa 静态库的链接补丁（放在 libdrm，不污染 libmesa）
 *
 * 1) renderonly：v3d_resource.c 在 ro=NULL 时仍然引用这两个符号；
 *    真 renderonly.c 需要 DRM dumb-buffer 模式设置接口，裸机用不上。
 * 2) v3d42_clif_dump_packet：clif_dump.c 同时引用 42/71 两套反汇编，
 *    我们只编了 71；42 分支永远走不到，给个空实现。
 */
#include <stdbool.h>
#include <stdint.h>

/* ---- renderonly stubs ---- */
struct renderonly_scanout;
struct renderonly;
struct pipe_resource;
struct winsys_handle;

void renderonly_scanout_destroy(struct renderonly_scanout* scanout,
                                struct renderonly* ro) {
  (void)scanout;
  (void)ro;
}

struct renderonly_scanout* renderonly_create_gpu_import_for_resource(
    struct pipe_resource* rsc, struct renderonly* ro,
    struct winsys_handle* out_handle) {
  (void)rsc;
  (void)ro;
  (void)out_handle;
  return 0;
}

/* ---- v3d 4.2 CLIF stub ---- */
bool v3d42_clif_dump_packet(void* clif, uint32_t offset, const uint8_t* cl,
                            uint32_t* size, bool reloc_mode) {
  (void)clif;
  (void)offset;
  (void)cl;
  (void)size;
  (void)reloc_mode;
  return false;
}
