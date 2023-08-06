#ifndef CLIENT_H
#define CLIENT_H

#define MAX_ARGS_BUF 128

enum {
  SYS_NEW_CLIENT = 1,
  SYS_GET_CLIENT = 2,
  SYS_DEL_CLIENT = 3,
};

enum{
  SYS_INIT=0,
  SYS_RUNNING=1,
  SYS_BLOCKING=2,
  SYS_WAIT=3,
  SYS_DINIT=4,
};

enum{
  API_INIT =0,
  API_REDAY=1,
  API_RETURN=2,
  API_FINISH=3
};

typedef struct api {
  int cid;        // call id
  int state;         // api state
  char* fn;       // function id
  void* ret;      // return value
  char args[MAX_ARGS_BUF];  // params
} api_t;


typedef struct client {
  char* name;
  int tid;
  int id;
  int token;
  int fd;
  int service_id;
  int type;
  int api_size;
  api_t* apis;
  int state;
} client_t;

typedef struct client_ctl {
  char* name;
  client_t* client;

} client_ctl_t;

typedef void* (*client_fn)(void* fn,void* args);

#endif