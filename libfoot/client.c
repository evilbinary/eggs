/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include "client.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

/* futex 操作码（不依赖 linux/futex.h，协议只有这两个） */
#define FOOT_FUTEX_WAIT 0
#define FOOT_FUTEX_WAKE 1

static uint32_t _call_seq = 1;

static uint64_t foot_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)(ts.tv_nsec / 1000000);
}

/* 共享页控制区：kick 字紧跟在槽数组后面（内核按同样布局留了 4 个字） */
static uint32_t* foot_kick(client_t* client) {
  return (uint32_t*)((char*)client->apis +
                     (long)client->api_size * (long)sizeof(api_t));
}

/* 在 addr 上等 ms 毫秒（0 = 只检查不睡）。返回 0 = 被唤醒/值已变，
 * -110 = 超时。timespec 用 16 字节布局（i64 sec + i64 nsec）。 */
static int foot_futex_wait(uint32_t* addr, uint32_t expect, unsigned int ms) {
  struct timespec ts;
  ts.tv_sec = (time_t)(ms / 1000);
  ts.tv_nsec = (long)(ms % 1000) * 1000000L;
  return (int)syscall(SYS_futex, addr, FOOT_FUTEX_WAIT, expect,
                      ms > 0 ? &ts : NULL, NULL, 0);
}

static void foot_futex_wake(uint32_t* addr, int n) {
  syscall(SYS_futex, addr, FOOT_FUTEX_WAKE, n, NULL, NULL, 0);
}

static api_t* foot_find_free_slot(client_t* client) {
  if (client == NULL || client->apis == NULL) return NULL;
  for (int i = 0; i < client->api_size; i++) {
    api_t* api = &client->apis[i];
    if (__atomic_load_n(&api->cid, __ATOMIC_ACQUIRE) == 0) {
      return api;
    }
  }
  return NULL;
}

/* 服务端处理函数的两代签名统一到这里 */
typedef struct {
  client_fn fn;     /* 旧签名（无长度） */
  client_fn_ex ex;  /* 新签名（带 payload 长度） */
} foot_handler_t;

static void* foot_invoke(foot_handler_t* h, api_t* api) {
  if (h->ex != NULL) {
    return h->ex(api->fn, api->args, api->len);
  }
  return h->fn(api->fn, api->args);
}

/* 服务端循环：轮询共享槽位。
 *  - state==API_REDAY 且 cid!=0：有请求，执行后置 API_RETURN 并 futex 唤醒调用方
 *  - state==API_RETURN：调用方已放弃（cid 被清零）或超过阈值未取 → 回收槽位
 *
 * 【所有状态迁移一律 CAS】调用方与服务方是并发进程，服务方"A 时刻看到的状态"
 * 到"写回去"之间，槽位可能已经被调用方取走结果、甚至放进一个新请求；
 * 直接 store 会把刚放下的请求静默抹掉（实测：32 次连续调用从第 3 次开始超时）。 */
static void foot_serve_slots(client_t* client, foot_handler_t* h, int* busy) {
  for (int i = 0; i < client->api_size; i++) {
    api_t* api = &client->apis[i];
    int st = __atomic_load_n(&api->state, __ATOMIC_ACQUIRE);

    if (st == API_REDAY) {
      int cid = __atomic_load_n(&api->cid, __ATOMIC_ACQUIRE);
      if (cid == 0) { /* 调用方已撤销：把槽位退回 INIT（CAS 不会覆盖新请求） */
        int expect = API_REDAY;
        __atomic_compare_exchange_n(&api->state, &expect, API_INIT, 0,
                                    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
        continue;
      }

      void* ret = foot_invoke(h, api);

      /* 处理期间调用方可能已超时撤销并把槽位交给新请求：cid 变了就丢弃结果 */
      if (__atomic_load_n(&api->cid, __ATOMIC_ACQUIRE) != cid) continue;

      __atomic_store_n(&api->err, 0, __ATOMIC_RELAXED);
      __atomic_store_n(&api->ret, ret, __ATOMIC_RELAXED);
      int expect = API_REDAY;
      if (__atomic_compare_exchange_n(&api->state, &expect, API_RETURN, 0,
                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        *busy = 1;
        foot_futex_wake((uint32_t*)&api->state, 1);
      }
    } else if (st == API_RETURN) {
      int cid = __atomic_load_n(&api->cid, __ATOMIC_RELAXED);
      uint32_t age = (uint32_t)(foot_now_ms() - api->claim_ms);
      if (cid == 0 || age > FOOT_SLOT_LEAK_MS) {
        int expect = API_RETURN;
        if (__atomic_compare_exchange_n(&api->state, &expect, API_INIT, 0,
                                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
          __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
        }
      }
    }
  }
}

int client_submit_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, foot_ticket_t* ticket) {
  if (client == NULL || ticket == NULL) {
    printf("foot submit: client/ticket is null\n");
    return -1;
  }

  api_t* api = foot_find_free_slot(client);
  if (api == NULL) {
    printf("foot submit: no free slot (api_size=%d)\n", client->api_size);
    return -2;
  }

  /* 参数区：清零后按实际长度拷贝（不再盲目拷 128/240 字节，避免读越界） */
  unsigned int len = (unsigned int)args_size;
  if (len > MAX_ARGS_BUF) len = MAX_ARGS_BUF;
  memset(api->args, 0, MAX_ARGS_BUF);
  if (args != NULL && len > 0) {
    memcpy(api->args, args, len);
  }
  api->len = len;
  api->fn = fn;
  api->err = 0;
  api->claim_ms = (unsigned int)foot_now_ms();

  uint32_t seq = __atomic_add_fetch(&_call_seq, 1, __ATOMIC_RELAXED);
  if (seq == 0) seq = __atomic_add_fetch(&_call_seq, 1, __ATOMIC_RELAXED);
  api->seq = seq;

  /* 先占槽（cid 从 0 改成 seq），再放行 state，服务端以 state 为门闩 */
  int expect = 0;
  if (!__atomic_compare_exchange_n(&api->cid, &expect, (int)seq, 0,
                                   __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
    printf("foot submit: slot busy\n");
    return -2;
  }
  __atomic_store_n(&api->state, API_REDAY, __ATOMIC_RELEASE);

  /* 通知服务端：kick 计数 +1（睡在 kick 上的服务端会被唤醒；
   * 服务端"先读 kick 再扫槽"的顺序保证这次提交不会被漏掉） */
  __atomic_add_fetch(foot_kick(client), 1, __ATOMIC_RELEASE);
  foot_futex_wake(foot_kick(client), 1);

  ticket->slot = (int)(api - client->apis);
  ticket->seq = seq;
  return 0;
}

void* client_wait_ex(client_t* client, foot_ticket_t* ticket,
                     unsigned int timeout_ms) {
  if (client == NULL || ticket == NULL) {
    printf("foot wait: client/ticket is null\n");
    return (void*)-1;
  }
  if (ticket->slot < 0 || ticket->slot >= client->api_size) {
    printf("foot wait: bad ticket slot %d\n", ticket->slot);
    return (void*)-1;
  }

  api_t* api = &client->apis[ticket->slot];
  uint64_t deadline = foot_now_ms() + (uint64_t)timeout_ms;

  for (;;) {
    /* 票据失效：槽位已被回收并被新请求复用（seq 变了），不能读别人的结果 */
    if (__atomic_load_n(&api->seq, __ATOMIC_ACQUIRE) != ticket->seq ||
        __atomic_load_n(&api->cid, __ATOMIC_ACQUIRE) == 0) {
      printf("foot wait: stale ticket slot=%d\n", ticket->slot);
      return (void*)-1;
    }

    int st = __atomic_load_n(&api->state, __ATOMIC_ACQUIRE);
    if (st == API_RETURN) {
      void* result = __atomic_load_n(&api->ret, __ATOMIC_ACQUIRE);
      /* 取走结果并归还槽位 */
      __atomic_store_n(&api->state, API_INIT, __ATOMIC_RELEASE);
      __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
      return result;
    }

    uint64_t now = foot_now_ms();
    if (now >= deadline) {
      /* 超时：能撤销就撤销（服务端尚未取走），否则放弃结果等它回收 */
      int expect_st = API_REDAY;
      if (__atomic_compare_exchange_n(&api->state, &expect_st, API_INIT, 0,
                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
        printf("foot wait: timeout (cancelled), slot=%d\n", ticket->slot);
        return (void*)-1;
      }
      /* 服务端已取走（处理中）或已发布结果：先放弃所有权（cid 变化会让服务端
       * 丢弃迟到结果），再把可能已发布的 RETURN 清回 INIT，否则槽位会一直
       * 挂在 RETURN 上，后续调用只能靠泄漏阈值兜底。 */
      __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
      expect_st = API_RETURN;
      __atomic_compare_exchange_n(&api->state, &expect_st, API_INIT, 0,
                                  __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
      printf("foot wait: timeout (abandoned), slot=%d\n", ticket->slot);
      return (void*)-1;
    }

    /* 睡在槽位的 state 上：服务端发布 RETURN 后会 futex 唤醒；
     * 值已经变了则 futex 立即返回 -EAGAIN，不会漏掉这次唤醒。 */
    {
      unsigned int wait_ms = (unsigned int)(deadline - now);
      if (wait_ms == 0) wait_ms = 1;
      foot_futex_wait((uint32_t*)&api->state, (uint32_t)st, wait_ms);
    }
  }
}

void* client_call_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, unsigned int timeout_ms) {
  foot_ticket_t ticket;
  int r = client_submit_ex(client, fn, args, args_size, &ticket);
  if (r < 0) {
    return (void*)-2;
  }
  return client_wait_ex(client, &ticket, timeout_ms);
}

static void foot_run_loop(client_t* client, foot_handler_t* h) {
  client->state = SYS_RUNNING;
  while (client->state == SYS_RUNNING) {
    /* 【顺序】先读 kick 再扫槽：扫槽期间到达的提交会改 kick，
     * 随后的 futex_wait 因值不匹配立即返回，不会睡死。 */
    uint32_t kick = __atomic_load_n(foot_kick(client), __ATOMIC_ACQUIRE);
    int busy = 0;
    foot_serve_slots(client, h, &busy);
    if (!busy) {
      /* 空闲期睡在 kick 上（带上限，兼顾 client->state 的退出条件） */
      foot_futex_wait(foot_kick(client), kick, FOOT_IDLE_WAIT_MS);
    }
  }
}

void client_run(client_t* client, client_fn fn) {
  if (client == NULL) {
    printf("error run client is null\n");
    return;
  }
  if (fn == NULL) {
    printf("error fn is null\n");
    return;
  }
  foot_handler_t h = {.fn = fn, .ex = NULL};
  printf("client start %s slots=%d\n", client->name, client->api_size);
  foot_run_loop(client, &h);
}

void client_run_ex(client_t* client, client_fn_ex fn) {
  if (client == NULL) {
    printf("error run client is null\n");
    return;
  }
  if (fn == NULL) {
    printf("error fn is null\n");
    return;
  }
  foot_handler_t h = {.fn = NULL, .ex = fn};
  printf("client start %s slots=%d\n", client->name, client->api_size);
  foot_run_loop(client, &h);
}

void client_run_one(client_t* client, client_fn fn) {
  if (client == NULL || fn == NULL) {
    printf("error run one client is null\n");
    return;
  }
  foot_handler_t h = {.fn = fn, .ex = NULL};
  client->state = SYS_RUNNING;
  int busy = 0;
  foot_serve_slots(client, &h, &busy);
}

client_t* client_regist_ex(char* name, int api_size) {
  int fd = open("/dev/foot", O_RDWR);
  if (fd < 0) {
    printf("client regist: open /dev/foot failed (%d)\n", fd);
    return NULL;
  }

  client_t* client = calloc(1, sizeof(client_t));
  if (client == NULL) {
    close(fd);
    return NULL;
  }
  client->name = name;
  client->api_size = api_size;

  client_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.name = name;
  ctl.api_size = api_size;
  ctl.client = client;

  int ret = ioctl(fd, SYS_NEW_CLIENT, &ctl);
  if (ret < 0) {
    printf("client regist %s error (%d)\n", name, ret);
    close(fd);
    free(client);
    return NULL;
  }
  client->fd = fd;
  printf("regist client %s id=%d slots=%d apis=%p\n", name, client->id,
         client->api_size, (void*)client->apis);
  return client;
}

client_t* client_get(char* name) {
  int fd = open("/dev/foot", O_RDWR);
  if (fd < 0) {
    printf("client get: open /dev/foot failed (%d)\n", fd);
    return NULL;
  }

  client_t* client = calloc(1, sizeof(client_t));
  if (client == NULL) {
    close(fd);
    return NULL;
  }
  client->name = name;

  client_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.name = name;
  ctl.client = client;

  int ret = ioctl(fd, SYS_GET_CLIENT, &ctl);
  if (ret < 0) {
    printf("client get %s error (%d)\n", name, ret);
    close(fd);
    free(client);
    return NULL;
  }
  client->fd = fd;
  printf("get client %s id=%d slots=%d apis=%p\n", name, client->id,
         client->api_size, (void*)client->apis);
  return client;
}

client_t* client_id(int id) {
  int fd = open("/dev/foot", O_RDWR);
  if (fd < 0) {
    printf("client get: open /dev/foot failed (%d)\n", fd);
    return NULL;
  }

  client_t* client = calloc(1, sizeof(client_t));
  if (client == NULL) {
    close(fd);
    return NULL;
  }

  client_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.id = id;
  ctl.client = client;

  int ret = ioctl(fd, SYS_GET_CLIENT_BY_ID, &ctl);
  if (ret < 0) {
    printf("client get id %d error (%d)\n", id, ret);
    close(fd);
    free(client);
    return NULL;
  }
  client->fd = fd;
  return client;
}

void clirent_un_regist(client_t* client) {
  if (client == NULL) {
    return;
  }
  if (client->fd >= 0) {
    close(client->fd);
  }
  free(client);
}
