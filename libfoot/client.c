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
#include <time.h>
#include <unistd.h>

#define FOOT_SPIN_LIMIT 4000 /* 前若干轮自旋，之后才退避睡 */

static uint32_t _call_seq = 1;

static uint64_t foot_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)(ts.tv_nsec / 1000000);
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

/* 服务端循环：轮询共享槽位。
 *  - state==API_REDAY 且 cid!=0：有请求，执行后置 API_RETURN
 *  - state==API_RETURN：调用方已放弃（cid 被清零）或超过阈值未取 → 回收槽位
 *
 * 【所有状态迁移一律 CAS】调用方与服务方是并发进程，服务方"A 时刻看到的状态"
 * 到"写回去"之间，槽位可能已经被调用方取走结果、甚至放进一个新请求；
 * 直接 store 会把刚放下的请求静默抹掉（实测：32 次连续调用从第 3 次开始超时）。 */
static void foot_serve_slots(client_t* client, client_fn fn, int* busy) {
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

      void* ret = fn(api->fn, api->args);

      /* 处理期间调用方可能已超时撤销并把槽位交给新请求：cid 变了就丢弃结果 */
      if (__atomic_load_n(&api->cid, __ATOMIC_ACQUIRE) != cid) continue;

      __atomic_store_n(&api->err, 0, __ATOMIC_RELAXED);
      __atomic_store_n(&api->ret, ret, __ATOMIC_RELAXED);
      int expect = API_REDAY;
      if (__atomic_compare_exchange_n(&api->state, &expect, API_RETURN, 0,
                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        *busy = 1;
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

void* client_call_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, unsigned int timeout_ms) {
  if (client == NULL) {
    printf("foot call: client is null\n");
    return (void*)-1;
  }

  api_t* api = foot_find_free_slot(client);
  if (api == NULL) {
    printf("foot call: no free slot (api_size=%d)\n", client->api_size);
    return (void*)-2;
  }

  /* 参数区：清零后按实际长度拷贝，避免读越界与残渣 */
  memset(api->args, 0, MAX_ARGS_BUF);
  if (args != NULL && args_size > 0) {
    if (args_size > MAX_ARGS_BUF) args_size = MAX_ARGS_BUF;
    memcpy(api->args, args, args_size);
  }
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
    printf("foot call: slot busy\n");
    return (void*)-2;
  }
  __atomic_store_n(&api->state, API_REDAY, __ATOMIC_RELEASE);

  uint64_t deadline = foot_now_ms() + timeout_ms;
  uint32_t spin = 0;
  void* result = NULL;

  for (;;) {
    int st = __atomic_load_n(&api->state, __ATOMIC_ACQUIRE);
    if (st == API_RETURN) {
      result = __atomic_load_n(&api->ret, __ATOMIC_ACQUIRE);
      break;
    }
    if ((spin & 0x3FF) == 0 && spin > 0 && foot_now_ms() >= deadline) {
      /* 超时：能撤销就撤销（服务端尚未取走），否则放弃结果等它回收 */
      int expect_st = API_REDAY;
      if (__atomic_compare_exchange_n(&api->state, &expect_st, API_INIT, 0,
                                      __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
        printf("foot call: timeout (cancelled), fn=%p\n", fn);
        return (void*)-1;
      }
      /* 服务端已取走（处理中）或已发布结果：先放弃所有权（cid 变化会让服务端
       * 丢弃迟到结果），再把可能已发布的 RETURN 清回 INIT，否则槽位会一直
       * 挂在 RETURN 上，后续调用只能靠泄漏阈值兜底。 */
      __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
      expect_st = API_RETURN;
      __atomic_compare_exchange_n(&api->state, &expect_st, API_INIT, 0,
                                  __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
      printf("foot call: timeout (abandoned), fn=%p\n", fn);
      return (void*)-1;
    }
    if (spin < FOOT_SPIN_LIMIT) {
      spin++;
      __asm__ __volatile__("yield");
    } else {
      spin++;
      usleep(FOOT_POLL_US);
    }
  }

  /* 取走结果并归还槽位 */
  __atomic_store_n(&api->state, API_INIT, __ATOMIC_RELEASE);
  __atomic_store_n(&api->cid, 0, __ATOMIC_RELEASE);
  return result;
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
  client->state = SYS_RUNNING;
  printf("client start %s slots=%d\n", client->name, client->api_size);
  while (client->state == SYS_RUNNING) {
    int busy = 0;
    foot_serve_slots(client, fn, &busy);
    if (!busy) {
      usleep(FOOT_POLL_US);
    }
  }
}

void client_run_one(client_t* client, client_fn fn) {
  if (client == NULL || fn == NULL) {
    printf("error run one client is null\n");
    return;
  }
  client->state = SYS_RUNNING;
  int busy = 0;
  foot_serve_slots(client, fn, &busy);
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
