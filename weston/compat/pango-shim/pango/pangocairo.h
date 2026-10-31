/*******************************************************************
 * Copyright 2021-present evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
/*
 * YiYiYa 的 pango 垫片（只够 clients/editor.c 用）。
 *
 * 【为什么需要】上游 clients/editor.c 是唯一一个把 pango 当硬依赖用的
 * toytoolkit 客户端（meson: deps = ['pangocairo', 'gobject-2.0']）。本平台的
 * cairo 是手工配置的（eggs/libcairo/src/cairo-features.h：只有 FT/toy/user
 * font，CAIRO_HAS_FC_FONT=0），既没有 pango 也没有 glib/gobject，所以
 * weston-editor 一直编不出来。
 *
 * 真移植 pango 要连带 GLib（GObject）、HarfBuzz、FriBidi、fontconfig 一大串，
 * 代价远大于它带来的价值。这里按 `eggs/weston/compat/` 里的既有套路
 * （drm-stubs.c / frame-stubs.c / libdisplay-info-stub.c）写一个**只实现
 * editor.c 用到的那一小撮 API**的垫片：
 *
 *   · 布局：只按 '\n' 断行，不做宽度换行（editor.c 从不调用
 *     pango_layout_set_width，上游默认就是 -1 = 不换行）；
 *   · 字体：用调用方 cairo_t 当前的 scaled font（cairo toy API），
 *     不解析 family/weight/size 描述；
 *   · 属性表（PangoAttrList/Attribute）：只做引用计数与排序存储，
 *     不参与绘制 —— 也就是说预编辑文本的下划线/配色不显示，字照样画。
 *   · UTF-8：索引一律按**字节**（pango 语义），逐码点量宽度，不会把多字节
 *     字符拆开。
 *
 * 用的时候只把这个目录加进 include path（`-I eggs/weston/compat/pango-shim`），
 * 这样 `#include <pango/pangocairo.h>` 命中垫片，而 compat/ 下的 cairo.h
 * 垫片（给 renderer-borders 用的空声明）不会顶掉真的 <cairo.h>。
 */
#ifndef YIYIYA_PANGO_SHIM_H
#define YIYIYA_PANGO_SHIM_H

#include <stdint.h>

#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 不透明类型 ---- */
typedef struct _PangoContext PangoContext;
typedef struct _PangoLayout PangoLayout;
typedef struct _PangoAttrList PangoAttrList;

/* PangoAttribute 必须是【完整类型】：editor.c 直接写 attr->start_index /
 * attr->end_index（上游 pango 里这两个也是公开字段）。这里把内部字段一并与
 * 实现共享，反正只有本垫片用它。 */
typedef struct _PangoAttribute {
  int type; /* 内部：ATTR_TYPE_* */
  unsigned int start_index;
  unsigned int end_index;
  unsigned short r;
  unsigned short g;
  unsigned short b;
  int ival; /* underline / weight */
} PangoAttribute;

/* ---- 枚举 / 结构 ---- */
typedef enum {
  PANGO_DIRECTION_LTR = 0,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_TTB,
  PANGO_DIRECTION_BTT,
  PANGO_DIRECTION_NEUTRAL,
} PangoDirection;

typedef enum {
  PANGO_UNDERLINE_NONE = 0,
  PANGO_UNDERLINE_SINGLE,
  PANGO_UNDERLINE_DOUBLE,
  PANGO_UNDERLINE_LOW,
  PANGO_UNDERLINE_ERROR,
} PangoUnderline;

typedef enum {
  PANGO_WEIGHT_NORMAL = 400,
  PANGO_WEIGHT_BOLD = 700,
} PangoWeight;

typedef struct {
  int x;
  int y;
  int width;
  int height;
} PangoRectangle;

/* ---- 单位换算（与上游同值：1 像素 = 1024 单位）---- */
#define PANGO_SCALE 1024
#define PANGO_PIXELS(d)                                              \
  ((int)((d) >= 0 ? ((int64_t)(d) + PANGO_SCALE / 2) / PANGO_SCALE   \
                  : ((int64_t)(d) - PANGO_SCALE / 2) / PANGO_SCALE))
#define PANGO_PIXELS_FLOOR(d) ((int)((int64_t)(d) / PANGO_SCALE))
#define PANGO_PIXELS_CEIL(d) \
  ((int)(((int64_t)(d) + PANGO_SCALE - 1) / PANGO_SCALE))

/* ---- layout 生命周期 / 属性 ---- */
PangoLayout* pango_cairo_create_layout(cairo_t* cr);
void pango_cairo_update_layout(cairo_t* cr, PangoLayout* layout);
void pango_cairo_show_layout(cairo_t* cr, PangoLayout* layout);

void pango_layout_set_text(PangoLayout* layout, const char* text, int length);
const char* pango_layout_get_text(PangoLayout* layout);
void pango_layout_set_attributes(PangoLayout* layout, PangoAttrList* attrs);
void pango_layout_get_extents(PangoLayout* layout, PangoRectangle* ink_rect,
                              PangoRectangle* logical_rect);
void pango_layout_get_cursor_pos(PangoLayout* layout, int index,
                                 PangoRectangle* strong_pos,
                                 PangoRectangle* weak_pos);
void pango_layout_xy_to_index(PangoLayout* layout, int x, int y, int* index,
                              int* trailing);
PangoContext* pango_layout_get_context(PangoLayout* layout);
void pango_context_set_base_dir(PangoContext* context, PangoDirection dir);

PangoAttrList* pango_attr_list_new(void);
PangoAttrList* pango_attr_list_ref(PangoAttrList* list);
void pango_attr_list_unref(PangoAttrList* list);
PangoAttrList* pango_attr_list_copy(PangoAttrList* list);
void pango_attr_list_insert(PangoAttrList* list, PangoAttribute* attr);

PangoAttribute* pango_attr_underline_new(PangoUnderline underline);
PangoAttribute* pango_attr_underline_color_new(unsigned short red,
                                               unsigned short green,
                                               unsigned short blue);
PangoAttribute* pango_attr_foreground_new(unsigned short red,
                                          unsigned short green,
                                          unsigned short blue);
PangoAttribute* pango_attr_background_new(unsigned short red,
                                          unsigned short green,
                                          unsigned short blue);
PangoAttribute* pango_attr_weight_new(PangoWeight weight);

/* ---- 给 editor.c 补的两个 glib 东西（真 pango 通过 <glib.h> 带进来） ----
 * g_utf8_offset_to_pointer：按"字符（码点）偏移"取指针，editor.c 用它把
 *   pango 返回的字节索引换成光标字节位置（负偏移要能往前走）。
 * g_clear_object：editor.c 拿它清 PangoLayout（上游把 layout 当 GObject 用），
 *   这里展开成"销毁 layout + 置空"。 */
char* g_utf8_offset_to_pointer(const char* str, long offset);
void pango_layout_destroy(PangoLayout* layout);
#define g_clear_object(pp)      \
  do {                          \
    if (*(pp) != NULL) {        \
      pango_layout_destroy(*(pp)); \
      *(pp) = NULL;             \
    }                           \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* YIYIYA_PANGO_SHIM_H */
