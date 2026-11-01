/* foot_qsort_r.c - GNU 版 qsort_r 的最小实现
 *
 * 本项目的 musl 版本没有 qsort_r（GNU 扩展，musl 1.1.20 才加入），
 * 而 Mesa 的 u_qsort.h 在 HAVE_GNU_QSORT_R 下直接调用它。
 * 这里用 heapsort 实现：无递归、无额外内存、最坏 O(n log n)。
 */
#include <stddef.h>
#include <string.h>

typedef int (*cmp_r_fn)(const void*, const void*, void*);

static void qswap(char* a, char* b, size_t size) {
  size_t i;
  for (i = 0; i < size; i++) {
    char t = a[i];
    a[i] = b[i];
    b[i] = t;
  }
}

/* 下沉：把 a[root] 沉到 size 范围的正确位置 */
static void sift_down(char* base, size_t root, size_t size, size_t width,
                      cmp_r_fn compar, void* arg) {
  for (;;) {
    size_t child = root * 2 + 1;
    if (child >= size) {
      break;
    }
    if (child + 1 < size &&
        compar(base + (child + 1) * width, base + child * width, arg) > 0) {
      child++;
    }
    if (compar(base + child * width, base + root * width, arg) <= 0) {
      break;
    }
    qswap(base + root * width, base + child * width, width);
    root = child;
  }
}

void qsort_r(void* base, size_t nmemb, size_t size,
             int (*compar)(const void*, const void*, void*), void* arg) {
  size_t i;

  if (base == NULL || nmemb < 2 || size == 0 || compar == NULL) {
    return;
  }

  /* 建堆（自底向上） */
  for (i = nmemb / 2; i > 0; i--) {
    sift_down((char*)base, i - 1, nmemb, size, compar, arg);
  }
  /* 逐个取出最大值放到末尾 */
  for (i = nmemb - 1; i > 0; i--) {
    qswap((char*)base, (char*)base + i * size, size);
    sift_down((char*)base, 0, i, size, compar, arg);
  }
}
