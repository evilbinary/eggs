#ifndef CLIENT_H
#define CLIENT_H

#define MAX_ARGS_BUF 240        /* 单个请求的 payload 上限（字节） */
#define MAX_INSTANCE 16
#define MAX_API_SLOTS 64        /* 每个 client 最多 64 个并发请求槽 */
#define FOOT_API_DEFAULT 4      /* 注册时默认申请 4 个槽 */
#define FOOT_SLOT_LEAK_MS 5000  /* 调用方崩溃后服务端回收槽位的阈值（ms） */
#define FOOT_CALL_TIMEOUT_MS 1000 /* 单次调用墙钟上限 */
#define FOOT_IDLE_WAIT_MS 500   /* 服务端空闲时睡在 kick 上的最长沉睡时间（ms） */

/* 共享页控制区（4 个字，紧跟在槽数组后面）：
 *   [0] kick  调用方的提交计数：每提交一个请求 +1 并 futex 唤醒；
 *             服务端空闲时睡在这个字上，醒来重扫槽位。
 *   [1..3]    预留（统计/心跳）。 */
#define FOOT_CTL_WORDS 4
#define FOOT_CTL_BYTES (FOOT_CTL_WORDS * 4)
#define FOOT_CTL_KICK 0

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
  unsigned int len;        /* args 有效长度（0..MAX_ARGS_BUF） */
  unsigned int flags;      /* 保留（同步/异步、唤醒策略等） */
  void* fn;                /* 方法号 */
  void* ret;               /* 返回值（只能是值，不能是跨进程指针） */
  char args[MAX_ARGS_BUF]; /* 参数区（按 len 拷贝，未用部分为 0） */
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

/* 服务方法：args 指向共享页里的参数区（只读，长度上限 MAX_ARGS_BUF）。
 * 返回值只能是"值"——两端是不同进程，指针不通用。 */
typedef void* (*client_fn)(void* fn, void* args);
/* 带长度版本：payload 长度由调用方按实际字节数写入（见 api_t.len）。 */
typedef void* (*client_fn_ex)(void* fn, void* args, unsigned int len);

client_t* client_regist_ex(char* name, int api_size);
client_t* client_get(char* name);
client_t* client_id(int id);
void clirent_un_regist(client_t* client);

void client_run(client_t* client, client_fn fn);
void client_run_ex(client_t* client, client_fn_ex fn);
void client_run_one(client_t* client, client_fn fn);

/* 返回值：服务端返回的值；-1 = 超时；-2 = 无空闲槽位；-3 = client 为空。 */
void* client_call_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, unsigned int timeout_ms);

/* 兼容旧写法：args 必须传栈上数组（宏取 sizeof 得到真实长度） */
#define client_regist(name) client_regist_ex((name), FOOT_API_DEFAULT)
#define client_call(client, fn, args)                                     \
  client_call_ex((client), (void*)(fn), (args), sizeof(args),             \
                 FOOT_CALL_TIMEOUT_MS)

#endif
