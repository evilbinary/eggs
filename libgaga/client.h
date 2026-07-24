#ifndef CLIENT_H
#define CLIENT_H

#define MAX_ARGS_BUF 128
#define MAX_INSTANCE 16

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

typedef struct api {
  int cid;                  // call id
  int state;                // api state
  char* fn;                 // function id
  void* ret;                // return value
  char args[MAX_ARGS_BUF];  // params
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
  client_t* client;
} client_ctl_t;

typedef void* (*client_fn)(void* fn, void* args);

client_t* client_regist(char* name);
void clirent_un_regist(client_t* client);

client_t* client_get(char* name);
client_t* client_id(int id);

void client_run_one(client_t* client, client_fn fn);

void client_run(client_t* client, client_fn fn);


#if defined(ARMV7_A) || defined(X86)
  #define lock_test_and_set __sync_lock_test_and_set
#else
  #define lock_test_and_set

#endif

#endif