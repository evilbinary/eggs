/*******************************************************************
 * Copyright 2021-present evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
/*
 * pango 垫片的实现（只覆盖 clients/editor.c 用到的接口，见同目录头文件
 * pango/pangocairo.h 的说明）。全部用 cairo toy 字体量宽度，不依赖
 * fontconfig / glib / harfbuzz。
 */
#include <stdlib.h>
#include <string.h>

#include <pango/pangocairo.h>

/* ---------------- 数据结构 ---------------- */

enum {
  ATTR_TYPE_UNDERLINE,
  ATTR_TYPE_UNDERLINE_COLOR,
  ATTR_TYPE_FOREGROUND,
  ATTR_TYPE_BACKGROUND,
  ATTR_TYPE_WEIGHT,
};

struct _PangoAttrList {
  int ref;
  PangoAttribute** attrs;
  int n;
  int cap;
};

struct _PangoContext {
  cairo_scaled_font_t* font;
  PangoDirection base_dir;
};

/* 一行文本的度量结果：字符边界（相对行首的字节偏移）+ 累计像素宽度 */
struct shim_line {
  int start;     /* 行首在 text 里的字节偏移 */
  int bytes;     /* 本行字节数（不含行尾 '\n'） */
  int nchars;    /* 字符（码点）个数 */
  int* char_off; /* nchars+1 个 */
  double* xpos;  /* nchars+1 个 */
};

struct _PangoLayout {
  char* text;
  int len;
  PangoAttrList* attrs;
  PangoContext* ctx;

  /* 下面是度量缓存（text / 字体变化时置 valid=0 重算） */
  int valid;
  struct shim_line* lines;
  int n_lines;
  double width;  /* px */
  double height; /* px */
  double ascent;
  double line_h;
};

/* ---------------- 小工具 ---------------- */

static int utf8_char_len(const unsigned char* s, int avail) {
  unsigned char c = *s;
  int n;
  if (c < 0x80) {
    return 1;
  } else if ((c & 0xe0) == 0xc0) {
    n = 2;
  } else if ((c & 0xf0) == 0xe0) {
    n = 3;
  } else if ((c & 0xf8) == 0xf0) {
    n = 4;
  } else {
    return 1; /* 非法字节：当单字节处理，别把布局算崩 */
  }
  return n <= avail ? n : 1;
}

static void ctx_refresh_metrics(PangoContext* ctx) {
  (void)ctx;
}

static PangoContext* ctx_create_from_cairo(cairo_t* cr) {
  PangoContext* ctx = calloc(1, sizeof(*ctx));
  if (ctx == NULL) {
    return NULL;
  }
  ctx->font = cairo_scaled_font_reference(cairo_get_scaled_font(cr));
  ctx->base_dir = PANGO_DIRECTION_LTR;
  return ctx;
}

static double font_line_height(PangoContext* ctx, double* ascent_out) {
  cairo_font_extents_t fe;
  double ascent = 0.0;
  double height = 0.0;

  if (ctx != NULL && ctx->font != NULL) {
    memset(&fe, 0, sizeof(fe));
    cairo_scaled_font_extents(ctx->font, &fe);
    ascent = fe.ascent;
    height = fe.ascent + fe.descent + fe.height * 0.0;
  }
  if (height <= 0.0) { /* 字体没量出来（没有字体文件的极端情况）时给个兜底 */
    ascent = 11.0;
    height = 16.0;
  }
  if (ascent_out != NULL) {
    *ascent_out = ascent;
  }
  return height;
}

/* 量一个 UTF-8 码点的前进宽度（px） */
static double char_advance(PangoContext* ctx, const char* s, int n) {
  cairo_text_extents_t ext;
  char buf[8];

  if (ctx == NULL || ctx->font == NULL) {
    return 8.0;
  }
  if (n <= 0 || n > (int)sizeof(buf) - 1) {
    return 8.0;
  }
  memcpy(buf, s, (size_t)n);
  buf[n] = '\0';
  memset(&ext, 0, sizeof(ext));
  cairo_scaled_font_text_extents(ctx->font, buf, &ext);
  return ext.x_advance > 0.0 ? ext.x_advance : 8.0;
}

/* ---------------- 布局计算 ---------------- */

static void layout_free_cache(PangoLayout* layout) {
  int i;
  if (layout->lines != NULL) {
    for (i = 0; i < layout->n_lines; i++) {
      free(layout->lines[i].char_off);
      free(layout->lines[i].xpos);
    }
    free(layout->lines);
  }
  layout->lines = NULL;
  layout->n_lines = 0;
  layout->valid = 0;
}

static void layout_compute(PangoLayout* layout) {
  const char* text = layout->text != NULL ? layout->text : "";
  int len = layout->len;
  int pos = 0;
  int nl = 0;
  double max_w = 0.0;
  double ascent = 0.0;

  if (layout->valid) {
    return;
  }
  layout_free_cache(layout);

  layout->line_h = font_line_height(layout->ctx, &ascent);
  layout->ascent = ascent;

  /* 先数行数：按 '\n' 断（上游默认 pango_layout_set_width(-1) = 不换行） */
  nl = 1;
  for (pos = 0; pos < len; pos++) {
    if (text[pos] == '\n') {
      nl++;
    }
  }
  layout->lines = calloc((size_t)nl, sizeof(struct shim_line));
  if (layout->lines == NULL) {
    layout->valid = 1;
    return;
  }

  pos = 0;
  for (int li = 0; li < nl; li++) {
    struct shim_line* ln = &layout->lines[li];
    int start = pos;
    int nchars = 0;

    while (pos < len && text[pos] != '\n') {
      nchars++;
      pos += utf8_char_len((const unsigned char*)text + pos, len - pos);
    }
    ln->start = start;
    ln->bytes = pos - start;
    ln->nchars = nchars;
    ln->char_off = calloc((size_t)nchars + 1, sizeof(int));
    ln->xpos = calloc((size_t)nchars + 1, sizeof(double));
    if (ln->char_off == NULL || ln->xpos == NULL) {
      ln->nchars = 0;
      continue;
    }
    {
      int ci = 0;
      int off = start;
      double x = 0.0;
      while (ci < nchars && off < start + ln->bytes) {
        int clen = utf8_char_len((const unsigned char*)text + off,
                                 start + ln->bytes - off);
        ln->char_off[ci] = off - start;
        ln->xpos[ci] = x;
        x += (text[off] == '\t') ? char_advance(layout->ctx, "    ", 4)
                                 : char_advance(layout->ctx, text + off, clen);
        ci++;
        off += clen;
      }
      ln->char_off[nchars] = ln->bytes;
      ln->xpos[nchars] = x;
      if (x > max_w) {
        max_w = x;
      }
    }
    if (pos < len && text[pos] == '\n') {
      pos++; /* 跳过行尾换行符 */
    }
  }

  layout->n_lines = nl;
  layout->width = max_w;
  layout->height = layout->line_h * (double)nl;
  layout->valid = 1;
}

/* ---------------- 公开接口：layout ---------------- */

PangoLayout* pango_cairo_create_layout(cairo_t* cr) {
  PangoLayout* layout;

  if (cr == NULL) {
    return NULL;
  }
  layout = calloc(1, sizeof(*layout));
  if (layout == NULL) {
    return NULL;
  }
  layout->ctx = ctx_create_from_cairo(cr);
  layout->text = strdup("");
  layout->len = 0;
  return layout;
}

void pango_cairo_update_layout(cairo_t* cr, PangoLayout* layout) {
  if (cr == NULL || layout == NULL || layout->ctx == NULL) {
    return;
  }
  if (layout->ctx->font != NULL) {
    cairo_scaled_font_destroy(layout->ctx->font);
  }
  layout->ctx->font = cairo_scaled_font_reference(cairo_get_scaled_font(cr));
  layout->valid = 0;
}

void pango_cairo_show_layout(cairo_t* cr, PangoLayout* layout) {
  double x0 = 0.0;
  double y0 = 0.0;
  char buf[8];

  if (cr == NULL || layout == NULL) {
    return;
  }
  layout_compute(layout);
  cairo_get_current_point(cr, &x0, &y0);

  for (int li = 0; li < layout->n_lines; li++) {
    struct shim_line* ln = &layout->lines[li];
    double baseline = y0 + layout->ascent + layout->line_h * (double)li;
    int off = ln->start;
    for (int ci = 0; ci < ln->nchars; ci++) {
      int clen = utf8_char_len((const unsigned char*)layout->text + off,
                               ln->start + ln->bytes - (off - ln->start));
      if (clen <= 0 || clen > (int)sizeof(buf) - 1) {
        break;
      }
      if (layout->text[off] != '\t') {
        memcpy(buf, layout->text + off, (size_t)clen);
        buf[clen] = '\0';
        cairo_move_to(cr, x0 + ln->xpos[ci], baseline);
        cairo_show_text(cr, buf);
      }
      off += clen;
    }
  }
  cairo_move_to(cr, x0, y0 + layout->height);
}

void pango_layout_set_text(PangoLayout* layout, const char* text, int length) {
  int len;

  if (layout == NULL) {
    return;
  }
  if (text == NULL) {
    text = "";
    len = 0;
  } else {
    len = length < 0 ? (int)strlen(text) : length;
  }
  free(layout->text);
  layout->text = malloc((size_t)len + 1);
  if (layout->text == NULL) {
    layout->text = NULL;
    layout->len = 0;
    return;
  }
  memcpy(layout->text, text, (size_t)len);
  layout->text[len] = '\0';
  layout->len = len;
  layout->valid = 0;
}

const char* pango_layout_get_text(PangoLayout* layout) {
  return layout != NULL ? layout->text : NULL;
}

void pango_layout_set_attributes(PangoLayout* layout, PangoAttrList* attrs) {
  if (layout == NULL) {
    return;
  }
  if (layout->attrs != NULL) {
    pango_attr_list_unref(layout->attrs);
  }
  layout->attrs = attrs != NULL ? pango_attr_list_ref(attrs) : NULL;
}

void pango_layout_get_extents(PangoLayout* layout, PangoRectangle* ink_rect,
                              PangoRectangle* logical_rect) {
  if (layout == NULL) {
    return;
  }
  layout_compute(layout);
  if (logical_rect != NULL) {
    logical_rect->x = 0;
    logical_rect->y = 0;
    logical_rect->width = (int)(layout->width * PANGO_SCALE);
    logical_rect->height = (int)(layout->height * PANGO_SCALE);
  }
  if (ink_rect != NULL) {
    /* 不区分 ink/logical（没有字形外框数据），给同一份值即可 */
    ink_rect->x = 0;
    ink_rect->y = 0;
    ink_rect->width = (int)(layout->width * PANGO_SCALE);
    ink_rect->height = (int)(layout->height * PANGO_SCALE);
  }
}

void pango_layout_get_cursor_pos(PangoLayout* layout, int index,
                                 PangoRectangle* strong_pos,
                                 PangoRectangle* weak_pos) {
  double x = 0.0;
  double y = 0.0;
  double h;

  if (layout == NULL) {
    return;
  }
  layout_compute(layout);
  h = layout->line_h;

  if (index < 0) {
    index = 0;
  }
  if (index > layout->len) {
    index = layout->len;
  }

  {
    int li = 0;
    for (int i = 0; i < layout->n_lines; i++) {
      struct shim_line* ln = &layout->lines[i];
      if (index >= ln->start && index <= ln->start + ln->bytes) {
        li = i;
        break;
      }
      li = i;
    }
    {
      struct shim_line* ln = &layout->lines[li];
      int rel = index - ln->start;
      int ci = 0;
      if (rel < 0) {
        rel = 0;
      }
      while (ci < ln->nchars && ln->char_off[ci + 1] <= rel) {
        ci++;
      }
      x = ln->xpos[ci];
      y = h * (double)li;
    }
  }

  if (strong_pos != NULL) {
    strong_pos->x = (int)(x * PANGO_SCALE);
    strong_pos->y = (int)(y * PANGO_SCALE);
    strong_pos->width = 0;
    strong_pos->height = (int)(h * PANGO_SCALE);
  }
  if (weak_pos != NULL) {
    *weak_pos = *strong_pos;
  }
}

void pango_layout_xy_to_index(PangoLayout* layout, int x, int y, int* index,
                              int* trailing) {
  double px = (double)x / PANGO_SCALE;
  double py = (double)y / PANGO_SCALE;
  int li;
  int result = 0;

  if (layout == NULL) {
    return;
  }
  layout_compute(layout);

  li = layout->line_h > 0.0 ? (int)(py / layout->line_h) : 0;
  if (li < 0) {
    li = 0;
  }
  if (li >= layout->n_lines) {
    li = layout->n_lines > 0 ? layout->n_lines - 1 : 0;
  }

  if (layout->n_lines > 0) {
    struct shim_line* ln = &layout->lines[li];
    int ci = 0;
    while (ci < ln->nchars && px >= ln->xpos[ci + 1]) {
      ci++;
    }
    result = ln->start + ln->char_off[ci];
  }

  if (index != NULL) {
    *index = result;
  }
  if (trailing != NULL) {
    *trailing = 0;
  }
}

PangoContext* pango_layout_get_context(PangoLayout* layout) {
  return layout != NULL ? layout->ctx : NULL;
}

void pango_context_set_base_dir(PangoContext* context, PangoDirection dir) {
  if (context != NULL) {
    context->base_dir = dir;
  }
}

/* ---------------- 公开接口：属性表（只做引用计数/排序，不参与绘制） ---------------- */

PangoAttrList* pango_attr_list_new(void) {
  PangoAttrList* list = calloc(1, sizeof(*list));
  if (list != NULL) {
    list->ref = 1;
  }
  return list;
}

PangoAttrList* pango_attr_list_ref(PangoAttrList* list) {
  if (list != NULL) {
    list->ref++;
  }
  return list;
}

static void attr_free(PangoAttribute* attr) { free(attr); }

void pango_attr_list_unref(PangoAttrList* list) {
  int i;
  if (list == NULL) {
    return;
  }
  if (--list->ref > 0) {
    return;
  }
  for (i = 0; i < list->n; i++) {
    attr_free(list->attrs[i]);
  }
  free(list->attrs);
  free(list);
}

PangoAttrList* pango_attr_list_copy(PangoAttrList* list) {
  PangoAttrList* out = pango_attr_list_new();
  if (out == NULL || list == NULL) {
    return out;
  }
  for (int i = 0; i < list->n; i++) {
    PangoAttribute* a = malloc(sizeof(*a));
    if (a == NULL) {
      break;
    }
    *a = *list->attrs[i];
    pango_attr_list_insert(out, a);
  }
  return out;
}

void pango_attr_list_insert(PangoAttrList* list, PangoAttribute* attr) {
  int i;
  if (list == NULL || attr == NULL) {
    return;
  }
  if (list->n == list->cap) {
    int cap = list->cap ? list->cap * 2 : 8;
    PangoAttribute** na =
        realloc(list->attrs, (size_t)cap * sizeof(PangoAttribute*));
    if (na == NULL) {
      attr_free(attr);
      return;
    }
    list->attrs = na;
    list->cap = cap;
  }
  /* 上游按 start_index 有序插入，这里照做（不影响绘制，只为语义一致） */
  i = list->n;
  while (i > 0 && list->attrs[i - 1]->start_index > attr->start_index) {
    list->attrs[i] = list->attrs[i - 1];
    i--;
  }
  list->attrs[i] = attr;
  list->n++;
}

static PangoAttribute* attr_new(int type) {
  PangoAttribute* a = calloc(1, sizeof(*a));
  if (a == NULL) {
    return NULL;
  }
  a->type = type;
  a->start_index = 0;
  a->end_index = 0xffffffffu;
  return a;
}

PangoAttribute* pango_attr_underline_new(PangoUnderline underline) {
  PangoAttribute* a = attr_new(ATTR_TYPE_UNDERLINE);
  if (a != NULL) {
    a->ival = (int)underline;
  }
  return a;
}

PangoAttribute* pango_attr_underline_color_new(unsigned short red,
                                               unsigned short green,
                                               unsigned short blue) {
  PangoAttribute* a = attr_new(ATTR_TYPE_UNDERLINE_COLOR);
  if (a != NULL) {
    a->r = red;
    a->g = green;
    a->b = blue;
  }
  return a;
}

PangoAttribute* pango_attr_foreground_new(unsigned short red,
                                          unsigned short green,
                                          unsigned short blue) {
  PangoAttribute* a = attr_new(ATTR_TYPE_FOREGROUND);
  if (a != NULL) {
    a->r = red;
    a->g = green;
    a->b = blue;
  }
  return a;
}

PangoAttribute* pango_attr_background_new(unsigned short red,
                                          unsigned short green,
                                          unsigned short blue) {
  PangoAttribute* a = attr_new(ATTR_TYPE_BACKGROUND);
  if (a != NULL) {
    a->r = red;
    a->g = green;
    a->b = blue;
  }
  return a;
}

PangoAttribute* pango_attr_weight_new(PangoWeight weight) {
  PangoAttribute* a = attr_new(ATTR_TYPE_WEIGHT);
  if (a != NULL) {
    a->ival = (int)weight;
  }
  return a;
}

/* ---------------- glib 补充 ---------------- */

/* 按"字符（码点）"偏移走：offset 可为负（glib 语义），非法/越界就停在边界 */
char* g_utf8_offset_to_pointer(const char* str, long offset) {
  const char* p = str != NULL ? str : "";

  if (offset > 0) {
    while (offset > 0 && *p != '\0') {
      p += utf8_char_len((const unsigned char*)p, 4);
      offset--;
    }
  } else {
    while (offset < 0 && p > str) {
      p--;
      while (p > str && ((unsigned char)*p & 0xc0) == 0x80) {
        p--;
      }
      offset++;
    }
  }
  return (char*)p;
}

void pango_layout_destroy(PangoLayout* layout) {
  if (layout == NULL) {
    return;
  }
  layout_free_cache(layout);
  if (layout->attrs != NULL) {
    pango_attr_list_unref(layout->attrs);
    layout->attrs = NULL;
  }
  if (layout->ctx != NULL) {
    if (layout->ctx->font != NULL) {
      cairo_scaled_font_destroy(layout->ctx->font);
    }
    free(layout->ctx);
    layout->ctx = NULL;
  }
  free(layout->text);
  free(layout);
}
