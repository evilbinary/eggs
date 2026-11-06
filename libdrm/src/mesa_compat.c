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

/* ---- ASTC 解压 stub（GLES2 clear/茶壶用不到 ASTC）---- */
void _mesa_unpack_astc_2d_ldr(unsigned char* dst_row, unsigned dst_stride,
                              const unsigned char* src_row,
                              unsigned src_stride, unsigned src_width,
                              unsigned src_height, unsigned format) {
  (void)dst_row; (void)dst_stride; (void)src_row; (void)src_stride;
  (void)src_width; (void)src_height; (void)format;
}

void _mesa_init_astc_decoder_luts(void* holder) { (void)holder; }

void* _mesa_get_astc_decoder_partition_table(unsigned int block_width,
                                             unsigned int block_height,
                                             unsigned int* lut_width,
                                             unsigned int* lut_height) {
  (void)block_width; (void)block_height;
  if (lut_width) *lut_width = 0;
  if (lut_height) *lut_height = 0;
  return 0;
}

void drisw_init_drawable(void* drawable, int isPixmap, int alphaBits) {
  (void)drawable; (void)isPixmap; (void)alphaBits;
}

void* tc_add_set_vertex_buffers_call(void* pipe, unsigned count) {
  (void)pipe; (void)count;
  return 0;
}

int pipe_loader_sw_probe(void* devs, int ndev) {
  (void)devs; (void)ndev;
  return 0;
}

void* tc_add_set_vertex_elements_and_buffers_call(void* pipe, unsigned count,
                                                  int account_for_unmaps) {
  (void)pipe; (void)count; (void)account_for_unmaps;
  return 0;
}

/* ---- DRI 软件光栅化（drisw）不编，留空实现 ---- */
void* drisw_init_screen(void* screen, int driver_name_is_inferred) {
  (void)screen; (void)driver_name_is_inferred;
  return 0;
}

/* ---- threaded context draw 快路径（未启用 tc，永不进入）---- */
void* tc_add_draw_single_call(void* pipe, void* index_bo) {
  (void)pipe; (void)index_bo;
  return 0;
}

/* ---- threaded context 查询 stub（未启用 u_threaded_context）---- */
int threaded_context_is_buffer_on_busy_list(void* pipe, void* resource) {
  (void)pipe; (void)resource;
  return 0;
}

/* ---- driver_trace dump 控制 stub：只在 GALLIUM_TRACE 调试时用 ---- */
void trace_dumping_start_locked(void) {}
void trace_dumping_stop_locked(void) {}
int trace_dumping_enabled_locked(void) { return 0; }

/* ---- HUD stub：GALLIUM_HUD 调试面板，运行时不需要 ---- */
struct hud_context;
struct hud_context* hud_create(void* cso, struct hud_context* share,
                               void* st, void* invalidate) {
  (void)cso; (void)share; (void)st; (void)invalidate;
  return 0;
}
void hud_destroy(struct hud_context* hud, void* cso) { (void)hud; (void)cso; }
void hud_run(struct hud_context* hud, void* cso, void* tex) {
  (void)hud; (void)cso; (void)tex;
}
void hud_record_only(struct hud_context* hud, void* pipe) {
  (void)hud; (void)pipe;
}
void hud_add_queue_for_monitoring(struct hud_context* hud, void* queue_info) {
  (void)hud; (void)queue_info;
}

/* cso_context.c 只做函数指针比较；没有 threaded context 时永不相等。 */
void tc_draw_vbo(void) {}

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
