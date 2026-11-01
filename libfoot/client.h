#ifndef CLIENT_H
#define CLIENT_H

#define MAX_ARGS_BUF 240        /* 单个请求的 payload 上限（字节） */
#define MAX_INSTANCE 16
#define MAX_API_SLOTS 64        /* 每个 client 最多 64 个并发请求槽 */
#define FOOT_API_DEFAULT 4      /* 注册时默认申请 4 个槽 */
#define FOOT_SLOT_LEAK_MS 5000  /* 调用方崩溃后服务端回收槽位的阈值（ms） */
#define FOOT_CALL_TIMEOUT_MS 1000 /* 单次调用墙钟上限 */
#define FOOT_IDLE_WAIT_MS 500   /* 服务端空闲时睡在 kick 上的最长沉睡时间（ms） */

/* 共享页控制区（8 个字，紧跟在槽数组后面；内核与用户态同布局）：
 *   [0] kick       调用方的提交计数：每提交一个请求 +1 并 futex 唤醒；
 *                  服务端空闲时睡在这个字上，醒来重扫槽位。
 *   [1] alive      服务端存活标记（注册时置 1，注销/崩溃清理时清 0）
 *   [2] req_total  提交过的请求总数
 *   [3] timeout    超时放弃次数
 *   [4] err        提交失败/票据失效次数
 *   [5] max_lat_ms 观测到的最大单次往返（ms）
 *   [6..7] 预留 */
#define FOOT_CTL_WORDS 8
#define FOOT_CTL_BYTES (FOOT_CTL_WORDS * 4)
#define FOOT_CTL_KICK 0
#define FOOT_CTL_ALIVE 1
#define FOOT_CTL_REQ 2
#define FOOT_CTL_TIMEOUT 3
#define FOOT_CTL_ERR 4
#define FOOT_CTL_MAX_LAT 5

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
  int map_count; /* 内核侧计数：共享页被多少个进程映射过（注销回收用） */
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

/* 未取结果的请求凭据：slot = 槽位号，seq = 提交时写入的序号。
 * 槽位被回收/复用时 seq 会变，据此识别失效票据（不会读到别人的结果）。 */
typedef struct {
  int slot;
  unsigned int seq;
} foot_ticket_t;

client_t* client_regist_ex(char* name, int api_size);
client_t* client_get(char* name);
client_t* client_id(int id);
void clirent_un_regist(client_t* client);

void client_run(client_t* client, client_fn fn);
void client_run_ex(client_t* client, client_fn_ex fn);
void client_run_one(client_t* client, client_fn fn);

/* 返回值：服务端返回的值；-1 = 超时/票据失效；-2 = 无空闲槽位。 */
void* client_call_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, unsigned int timeout_ms);

/* 【非阻塞提交】只把请求放进共享槽就返回，不等结果：
 *   0 = 已提交（ticket 可交给 client_wait_ex）
 *  -1 = 参数非法   -2 = 无空闲槽位
 * 图形栈可以攒一帧的 N 条命令一次提交，再逐条/批量收结果。 */
int client_submit_ex(client_t* client, void* fn, const void* args,
                     unsigned long args_size, foot_ticket_t* ticket);
/* 等某个已提交请求的结果：-1 = 超时或票据失效（槽位被回收/复用）。 */
void* client_wait_ex(client_t* client, foot_ticket_t* ticket,
                     unsigned int timeout_ms);

/* 【可观测性】共享页里的统计计数器（零系统调用，直接读页）：
 * 请求总数、超时数、错误数、观测到的最大往返。 */
typedef struct {
  unsigned int req_total;
  unsigned int timeout_total;
  unsigned int err_total;
  unsigned int max_latency_ms;
  unsigned int alive; /* 服务端是否还活着 */
} foot_stats_t;

int client_stats(client_t* client, foot_stats_t* out); /* 0 / -1 */

/* ===================== capability（阶段 4） =====================
 * 用户态驱动不再自己填物理地址去映射：先按名字申请一个能力句柄
 * （内核查设备窗口表），再用句柄映射窗口内的页。句柄只在本进程有效，
 * 伪造/过期的句柄会被内核按 gen 拒掉。
 *
 * 【与内核一致性】下面的命令号/权限位/控制块布局必须与
 * duck/modules/foot/service.h 的同名定义逐字段一致（两侧刻意各存一份）。 */
enum {
  SYS_CAP_GET_MMIO = 5, /* ctl.name → ctl.handle */
  SYS_CAP_MAP = 6,      /* handle + vaddr + offset + len */
  SYS_CAP_UNMAP = 7,
  SYS_CAP_INFO = 8,     /* → pa/size/rights */
  SYS_CAP_DERIVE = 9,   /* handle + rights → handle */
  SYS_CAP_REVOKE = 10,
  SYS_CAP_ALLOC_FRAME = 11, /* size/pa → handle（输出 pa/size/rights） */
  /* 端点（IPC）：创建 / 投递（可携带能力）/ 接收。
   * 这三个命令的参数结构是 foot_ep_ctl_t，不是 foot_cap_ctl_t。 */
  SYS_CAP_EP_CREATE = 12,
  SYS_CAP_EP_SEND = 13,
  SYS_CAP_EP_RECV = 14,
  /* 物理连续大块（OBJ_ARENA）：foot_cap_ctl_t 的 size=长度、offset=物理对齐。
   * 输出 handle + pa + size。GPU 页表（4MB 连续）/ QPU 代码 / uniform 用。 */
  SYS_CAP_ALLOC_ARENA = 15,
};

/* 通用权限（低 16 位）：与内核 duck/kernel/cap.h 的 RIGHT_* 同值 */
enum {
  CAP_RIGHT_LOAD = 1 << 0,    /* 读对象状态/数据 */
  CAP_RIGHT_STORE = 1 << 1,   /* 改对象状态/数据 */
  CAP_RIGHT_INVOKE = 1 << 2,  /* 调用对象方法 */
  CAP_RIGHT_GRANT = 1 << 3,   /* 复制给其它 CSpace */
  CAP_RIGHT_MINT = 1 << 4,    /* 派生受限能力 */
  CAP_RIGHT_REVOKE = 1 << 5,  /* 撤销派生能力 */
  CAP_RIGHT_DESTROY = 1 << 6, /* 销毁对象 */
  CAP_RIGHT_CREATE = 1 << 7,  /* 基于对象创建子对象 */
};

/* 类型位（高 16 位）：按对象类型解释，不同类型可复用同一位 */
enum {
  CAP_DEVWIN_MAP = 1 << 16,     /* 允许映射窗口进地址空间 */
  CAP_DEVWIN_MAILBOX = 1 << 17, /* 允许直接读写寄存器 */
  CAP_FRAME_MAP = 1 << 16,      /* FRAME：允许把该物理页映射进地址空间 */
  CAP_FRAME_UNMAP = 1 << 17,    /* FRAME：允许解除映射 */
  CAP_EP_SEND = 1 << 16,        /* ENDPOINT：允许投递消息 */
  CAP_EP_RECV = 1 << 17,        /* ENDPOINT：允许接收消息 */
  CAP_EP_NOTIFY = 1 << 18,      /* ENDPOINT：允许通知（预留） */
};

typedef struct foot_cap_ctl {
  char* name;          /* GET_MMIO：窗口名 */
  unsigned int handle; /* 句柄（入参或输出） */
  unsigned int vaddr;  /* MAP：目标 VA */
  unsigned int offset; /* MAP：窗口内偏移 */
  unsigned int len;    /* MAP：长度 */
  unsigned int rights; /* DERIVE/INFO */
  unsigned int pa;     /* INFO 输出 */
  unsigned int size;   /* INFO 输出 */
} foot_cap_ctl_t;

typedef unsigned int foot_cap_t;
#define FOOT_CAP_NONE 0u

/* 申请设备窗口（如 "gpio" / "dwc2" / "bcm-periph"）；失败返回 FOOT_CAP_NONE */
foot_cap_t foot_cap_get_mmio(const char* name);
/* 把窗口内 [offset, offset+len) 映射到 vaddr（页对齐）；0 成功，-1 非法 */
int foot_cap_map(foot_cap_t cap, void* vaddr, unsigned long offset,
                 unsigned long len);
/* 解除最近一次映射（完整清理仍由进程退出的页表销毁兜底） */
int foot_cap_unmap(foot_cap_t cap);
/* 查窗口物理基址/长度/当前权限（指针可传 NULL） */
int foot_cap_info(foot_cap_t cap, unsigned int* pa, unsigned int* size,
                  unsigned int* rights);
/* 派生：rights 必须是原权限子集，且原 cap 带 GRANT 位 */
foot_cap_t foot_cap_derive(foot_cap_t cap, unsigned int rights);
/* 撤销（连同派生出的子 cap），已建立的映射一并解除 */
int foot_cap_revoke(foot_cap_t cap);

/* 申请物理页帧对象：size==0 表示一页（按页向上取整，上限 256KB）；
 * pa==0 从页池新分配（对象负责归还），非 0 则包装该物理地址（不接管）；
 * rights==0 取默认（读 + 可映射 + 可派生/撤销/转授）。失败返回 FOOT_CAP_NONE。
 * 成功后 pa/size（指针可传 NULL）回填实际物理基址与长度。 */
foot_cap_t foot_cap_alloc_frame(unsigned long size, unsigned long pa,
                               unsigned int rights, unsigned int* out_pa,
                               unsigned int* out_size);

/* 申请物理【连续】大块对象（OBJ_ARENA）：size 按页向上取整（上限 16MB）；
 * alignment 是物理对齐（0=页对齐，须为 2 的幂）；rights==0 取默认。
 * 走 phy_alloc_aligment，一次拿整块物理连续 + 恒等映射（GPU 页表 4MB 连续）。
 * 失败返回 FOOT_CAP_NONE；成功回填 pa/size（指针可传 NULL）。 */
foot_cap_t foot_cap_alloc_arena(unsigned long size, unsigned long alignment,
                                unsigned int rights, unsigned int* out_pa,
                                unsigned int* out_size);

/* ---- IPC 端点：消息 + 随消息携带能力 ----
 * fork 之后父子共享同一个 endpoint（CSpace 整表复制），因此它就是
 * "父进程把自己的一个能力交给子进程"的通路：
 *   父 foot_cap_ep_send(ep, buf, n, some_cap) → 子 foot_cap_ep_recv(ep, ...)
 * 取到的是一个【新句柄】，可以直接拿去做 map/info。
 * 队列深度 8 条、单条数据 64 字节；满/空返回 -1（非阻塞，需轮询）。 */
typedef struct foot_ep_ctl {
  unsigned int handle;    /* endpoint 句柄 */
  unsigned int grant;     /* SEND：要转授的句柄；RECV：输出收到的句柄 */
  unsigned int len;       /* SEND：数据长度；RECV：缓冲区容量 */
  unsigned int out_len;   /* RECV：实际长度 */
  unsigned int rights;    /* SEND：给接收方的权限（0 = 沿用原权限） */
  unsigned int flags;     /* 保留 */
  char* buf;              /* 数据缓冲区 */
} foot_ep_ctl_t;

/* flags 位：非阻塞（见 foot_ep_ctl_t.flags） */
#define FOOT_EP_NONBLOCK 1u

/* 新建 endpoint；失败返回 FOOT_CAP_NONE */
foot_cap_t foot_cap_ep_create(void);
/* 投递（【阻塞】直到队列有空位）：0 成功，-1 失败（无 EP_SEND/无转授权） */
int foot_cap_ep_send(foot_cap_t ep, const void* data, unsigned long len,
                     foot_cap_t grant);
/* 接收（【阻塞】直到有消息）：0 成功，-1 失败（无 EP_RECV）；out 可传 NULL */
int foot_cap_ep_recv(foot_cap_t ep, void* data, unsigned long cap,
                     unsigned long* out_len, foot_cap_t* out_grant);
/* 非阻塞版本：队列满/空立刻返回 -1，调用方自己轮询 */
int foot_cap_ep_try_send(foot_cap_t ep, const void* data, unsigned long len,
                         foot_cap_t grant);
int foot_cap_ep_try_recv(foot_cap_t ep, void* data, unsigned long cap,
                         unsigned long* out_len, foot_cap_t* out_grant);

/* 兼容旧写法：args 必须传栈上数组（宏取 sizeof 得到真实长度） */
#define client_regist(name) client_regist_ex((name), FOOT_API_DEFAULT)
#define client_call(client, fn, args)                                     \
  client_call_ex((client), (void*)(fn), (args), sizeof(args),             \
                 FOOT_CALL_TIMEOUT_MS)

/* ===================== dcache 维护（内核自定义号 522） =====================
 * 用户态驱动（GPU g2dd 等）dispatch 前后要对 dcache 做 clean / invalidate。
 * aarch64 上 EL0 只能 dc civac，纯 invalidate 要 EL1 ⇒ 统一走 cacheflush。 */
#define FOOT_SYS_CACHEFLUSH 522

#define FOOT_CACHE_CLEAN 0
#define FOOT_CACHE_INVALIDATE 1
#define FOOT_CACHE_CLEAN_INV 2

/* 半开区间 [start, end)。clean 用 clean+invalidate（dc civac 语义）。 */
void foot_cache_clean(void* start, void* end);
void foot_cache_invalidate(void* start, void* end);

#endif
