#ifndef CLIENT_H
#define CLIENT_H

#define MAX_ARGS_BUF 128
#define MAX_INSTANCE 16
#define MAX_API_SLOTS 8      /* 每个 client 最多 8 个并发请求槽 */
#define FOOT_API_DEFAULT 4   /* 注册时默认申请 4 个槽 */
#define FOOT_SLOT_LEAK_MS 5000 /* 调用方崩溃后服务端回收槽位的阈值（ms） */
#define FOOT_CALL_TIMEOUT_MS 1000 /* 单次调用墙钟上限 */
#define FOOT_POLL_US 50      /* 退避粒度（后续换 futex 唤醒） */

enum {
  SYS_NEW_CLIENT = 1,
  SYS_GET_CLIENT = 2,
  SYS_DEL_CLIENT = 3,
  SYS_GET_CLIENT_BY_ID = 4,
};

enum {
  SYS_INIT = 0,
  SYS_RUNNING = 1,
  SYS_BLOCKING = 2,
  SYS_WAIT = 3,
  SYS_DINIT = 4,
};

enum { API_INIT = 0, API_REDAY = 1, API_RETURN = 2, API_FINISH = 3 };

/* 共享页里的一个请求槽。
 * 【重要】本结构与 duck/modules/foot/service.h 的同名结构必须逐字段一致：
 * 内核与用户态各自持有一份定义（刻意解耦），字段变更时两侧同批修改。 */
typedef struct api {
  int cid;                 /* 0 = 空闲，非 0 = 正在处理（值即 seq） */
  int state;               /* API_INIT / API_REDAY / API_RETURN / API_FINISH */
  int err;                 /* 响应错误码：0 = OK */
  unsigned int seq;        /* 请求序号，用于识别陈旧响应 */
  unsigned int claim_ms;   /* 调用方写入的提交时刻（ms），服务端据此回收僵尸槽 */
  void* fn;                /* 方法号 */
  void* ret;               /* 返回值 */
  char args[MAX_ARGS_BUF]; /* 参数区（按实际长度拷贝） */
} api_t;

typedef struct client {
  char* name;
  int tid;
  int id;
  int cid;
  int token;
  int fd;
  int service_id;
  int type;
  int api_size;
  api_t* apis;
  int state;
  int count;
  struct client* instance[MAX_INSTANCE];
} client_t;

typedef struct client_ctl {
  char* name;
  int id;
  int api_size; /* SYS_NEW_CLIENT：期望槽位数；返回时由内核回填 */
  client_t* client;
} client_ctl_t;

typedef void* (*client_fn)(void* fn, void* args);

client_t* client_regist_ex(char* name, int api_size);
client_t* client_get(char* name);
client_t* client_id(int id);
void clirent_un_regist(client_t* client);

void client_run(client_t* client, client_fn fn);
void client_run_one(client_t* client, client_fn fn);

void* client_call_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, unsigned int timeout_ms);

/* 兼容旧写法：args 必须传栈上数组（宏取 sizeof 得到真实长度） */
#define client_regist(name) client_regist_ex((name), FOOT_API_DEFAULT)
#define client_call(client, fn, args)                                     \
  client_call_ex((client), (void*)(fn), (args), sizeof(args),             \
                 FOOT_CALL_TIMEOUT_MS)

#endif
