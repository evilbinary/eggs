#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "assert.h"
#include "sys/stat.h"
#include "syscall.h"
#include "time.h"
#include "yiyiya.h"

#define PAGE_SIZE 4096
#define DEBUG 1

static int page_size = -1;
static char* last_heap_addr = NULL;
const size_t align_to = 16;

void* ya_sbrk(size_t size) {
  if (last_heap_addr == NULL) {
    last_heap_addr = syscall0(SYS_VHEAP);
  }
  void* addr = last_heap_addr;
  last_heap_addr += size;
  return addr;
}

typedef struct block {
  size_t size;
  struct block* next;
  struct block* prev;
  char free;
  u32 magic;
} block_t;

block_t* g_block_list = NULL;
block_t* g_block_list_last = NULL;

#define ya_block_ptr(ptr) ((block_t*)ptr - 1);
#define ya_block_addr(ptr) ((block_t*)ptr + 1);

#define MAGIC_FREE 999999999
#define MAGIC_USE 888888888

block_t* ya_new_block(size_t size) {
  block_t* block = ya_sbrk(size + sizeof(block_t));
  block->free = 0;
  block->next = NULL;
  block->prev = NULL;
  block->size = size;
  block->magic = MAGIC_USE;
  if (g_block_list == NULL) {
    g_block_list = block;
    g_block_list_last = block;
  } else {
    g_block_list_last->next = block;
    block->prev = g_block_list_last;
    g_block_list_last = block;
  }
  return block;
}

block_t* ya_find_free_block(size_t size) {
  block_t* block = g_block_list;
  block_t* find_block = NULL;
  while (block) {
    if (block->free == 1 && block->size >= size) {
      find_block = block;
      break;
    }
    block = block->next;
  }
  if (find_block == NULL) {
    find_block = ya_new_block(size);
  }
  return find_block;
}

void* ya_alloc(size_t size) {
  if (size <= 0) {
    return NULL;
  }
  block_t* block;
  if (g_block_list == NULL) {
    block = ya_new_block(size);
  } else {
    block = ya_find_free_block(size);
  }
  block->free = 0;
  block->magic = MAGIC_USE;
  void* addr = ya_block_addr(block);
#ifdef DEBUG
  printf("alloc %x size=%d\n", addr, size);
#endif
  return addr;
}

void ya_free(void* ptr) {
  if (ptr == NULL) {
    return;
  }
  block_t* block = ya_block_ptr(ptr);
#ifdef DEBUG
  printf("free  %x size=%d\n", ptr, block->size);
#endif
  assert(block->free == 0);
  assert(block->magic == MAGIC_USE);
  block->free = 1;
  block->magic = MAGIC_FREE;
  block_t* next = block->next;
  if (next != NULL) {
    if (next->free == 1) {
      block->size += next->size + sizeof(block_t);
      block->next = next->next;
      int size = next->size;
      if (next->next) {
        next->next->prev = block;
      }
      // memset(next,0,size);
    }
  }
  block_t* prev = block->prev;
  if (prev != NULL) {
    if (prev->free == 1) {
      prev->size += block->size;
      prev->next = block->next;
      if (block->next != NULL) {
        block->next->prev = prev;
      }
      int size = block->size;
      // memset(block,0,size);
    }
  }
}

void* ya_realloc(void* p, size_t size) {
  if (p == NULL) {
    return malloc(size);
  }
  block_t* block = ya_block_ptr(p);
  if (block->size >= size) {
    return p;
  }
  void* new_p = malloc(size);
  memcpy(new_p, p, block->size);
  free(p);
  return new_p;
}

void* ya_calloc(size_t nobj, size_t nsize) {
  size_t size = nobj * nsize;
  void* ptr = malloc(size);
  memset(ptr, 0, size);
  return ptr;
}

size_t ya_real_size(void* ptr) {
  block_t* block = ya_block_ptr(ptr);
  return block->size;
}

// typedef struct free_block {
//   size_t size;
//   struct free_block* next;
// } free_block_t;

// free_block_t free_block_list_head = {.size = 0, .next = 0};

// void* ya_alloc(size_t size) {
//   size_t real_size = (size + sizeof(free_block_t) + (align_to - 1)) &
//   ~(align_to - 1); free_block_t* block = free_block_list_head.next;
//   free_block_t** head = &(free_block_list_head.next);
//   while (free_block_list_head.size != 0 && block != 0) {
//     if (block->size >= size) {
//       *head = block->next;
//       void* addr = ((char*)block) + sizeof(free_block_t);
//       // memset(addr,0,s);
//       return addr;
//     }
//     head = &(block->next);
//     block = block->next;
//   }

//   block = (free_block_t*)ya_sbrk(real_size);
//   block->size = size;
//   block->next=NULL;

//   void* addr = ((char*)block) + sizeof(free_block_t);
//   // memset(addr,0,s);
//   return addr;
// }

// void ya_free(void* ptr) {
//   free_block_t* block = (free_block_t*)(((char*)ptr) - sizeof(free_block_t));
//   block->next = free_block_list_head.next;
//   free_block_list_head.next = block;
//   void* addr = ((char*)block) + sizeof(free_block_t);
//   // memset(addr,0,block->size);
// }

// size_t ya_real_size(void* ptr){
//   free_block_t* block = (free_block_t*)(((char*)ptr) - sizeof(free_block_t));
//   int size=block->size;
//   // size = (size + (align_to - 1)) & ~(align_to - 1);
//   return size;
// }

// void* ya_realloc(void* p, size_t size) {
//   void* ptr;
//   if (size == 0) {
//     free(p);
//     return NULL;
//   }
//   if (p == NULL) return malloc(size);
//   int real_size = ya_real_size(p);
//   ptr = malloc(size);
//   memcpy(ptr, p, real_size);
//   free(p);
//   // if (size > real_size) {
//   //   ptr = malloc(size);
//   //   memcpy(ptr, p, real_size);
//   //   free(p);
//   // }
//   return ptr;
// }

// void* ya_calloc(size_t nobj, size_t size) {
//   int malloc_size = nobj * size;
//   void* p = malloc(malloc_size);
//   int real_size = malloc_usable_size(p);
//   memset(p, 0, malloc_size);
//   return p;
// }