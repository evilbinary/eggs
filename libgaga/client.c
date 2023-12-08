/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include "client.h"

#include "fcntl.h"
#include "stdarg.h"
#include "stdio.h"

void* client_call(client_t* client, void* fn, ...) {
  if (client == NULL) {
    printf("call client is null\n");
    return NULL;
  }
  printf("call client %s\n", client->name);
  api_t* apis = client->apis;
  if (apis == NULL) {
    printf("api is null\n");
    return NULL;
  }
  api_t* api = NULL;
  for (int i = 0; i < client->api_size; i++) {
    api = &apis[i];
    if (api->cid == 0) {
      break;
    }
  }
  api->cid = client->id;
  api->fn = fn;

  void* arg;
  va_list ap;
  va_start(ap, fn);
  arg = va_arg(ap, void*);
  va_end(ap);

  memmove(api->args, arg, MAX_ARGS_BUF);
  // api->state = API_REDAY;
  __sync_lock_test_and_set(&api->state, API_REDAY);
  printf("call client %s ready api state %d\n", client->name, api->state);
  int count = 0;
  while (api->state == API_REDAY) {
    count++;
    if (count > 100000000 && client->count > 0) {
      // todo get client stragtegy
      client_t* c = client->instance[1];
      count = 0;
      return client_call(c, fn, arg);
    }
  }
  printf("client %s state: %d\n", client->name, api->state);
  // api->state = API_FINISH;
  __sync_lock_test_and_set(&api->state, API_FINISH);
  return api->ret;
}

void* client_ret(client_t* client, void* fn, ...) { return 0; }

void* client_acall(client_t* client, void* fn, ...) { return 0; }

void* client_aret(client_t* client, void* fn, ...) { return 0; }

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
  printf("client start %s\n", client->name);
  while (1) {
    api_t* api = NULL;
    for (int i = 0; i < client->api_size; i++) {
      api = &client->apis[i];
      if (api->state == API_REDAY) {
        printf("client run %s state: %d\n", client->name, api->state);
        __sync_lock_test_and_set(&api->ret, fn(api->fn, api->args));
        __sync_lock_test_and_set(&api->state, API_RETURN);
        // api->ret = fn(api->fn, api->args);
        // api->state = API_RETURN;
      }
    }
  }
}

void client_run_one(client_t* client, client_fn fn) {
  if (client == NULL) {
    printf("error run one client is null\n");
    return;
  }
  if (fn == NULL) {
    printf("error fn is null\n");
    return;
  }
  client->state = SYS_RUNNING;
  api_t* api = NULL;
  for (int i = 0; i < client->api_size; i++) {
    api = &client->apis[i];
    if (api->state == API_REDAY) {
      printf("client run %s state: %d\n", client->name, api->state);
      __sync_lock_test_and_set(&api->ret, fn(api->fn, api->args));
      __sync_lock_test_and_set(&api->state, API_RETURN);
      // api->ret = fn(api->fn, api->args);
      // api->state = API_RETURN;
    }
  }
}

client_t* client_regist(char* name) {
  int fd = open("/dev/gaga", O_RDWR);
  printf("client open fd:%d\n", fd);
  if (fd < 0) {
    printf("client regist error\n");
    return NULL;
  }
  client_t* client = malloc(sizeof(client_t));
  client->name = name;
  client_ctl_t ctl;
  ctl.name = name;
  ctl.client = client;
  int ret = ioctl(fd, SYS_NEW_CLIENT, &ctl);
  printf("regist client %s api_size: %d apis: %x\n", client->name,
         client->api_size, client->apis);
  if (ret < 0) {
    printf("client regist get error\n");
    free(client);
    return NULL;
  }
  client->fd = fd;
  return client;
}

client_t* client_get(char* name) {
  int fd = open("/dev/gaga", O_RDWR);
  printf("client open fd:%d\n", fd);
  if (fd < 0) {
    printf("client get open %s error\n", name);
    return NULL;
  }
  client_t* client = malloc(sizeof(client_t));

  client_ctl_t ctl;
  client->name = name;
  ctl.name = name;
  ctl.client = client;
  int ret = ioctl(fd, SYS_GET_CLIENT, &ctl);
  if (ret < 0) {
    printf("client get ioctl %s error\n", name);
    free(client);
    return NULL;
  }
  printf("get client name %s %x id %d \n", client->name, client, client->id);
  return client;
}

client_t* client_id(int id) {
  int fd = open("/dev/gaga", O_RDWR);
  printf("client open fd:%d\n", fd);
  if (fd < 0) {
    printf("client get open %d error\n", id);
    return NULL;
  }
  client_t* client = malloc(sizeof(client_t));

  client_ctl_t ctl;
  client->name = NULL;
  ctl.id = id;
  ctl.client = client;
  int ret = ioctl(fd, SYS_GET_CLIENT_BY_ID, &ctl);
  if (ret < 0) {
    printf("client get ioctl %d error\n", id);
    free(client);
    return NULL;
  }
  printf("get client %x id %d \n",client, client->id);
  return client;
}

void clirent_un_regist(client_t* client) {
  if (client == NULL) {
    return;
  }
  int ret = close(client->fd);
  free(client);
}

