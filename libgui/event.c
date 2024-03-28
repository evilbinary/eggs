#include "event.h"

#include "joystick.h"
#include "stdlib.h"
#include "syscall.h"

event_t null_event;
#define ARRAY_QUEUE_TYPE event_t
#define ARRAY_QUEUE_TYPE_NULL null_event

#include "algorithm/array_queue.h"

aqueue_t queue;
event_info_t event_info = {0};
static mouse_data_t current_mouse;
static u32 button_press = 0;

u8 scan_code;
u8 shf_p = 0;
u8 ctl_p = 0;
u8 alt_p = 0;
u32 col = 0;
u32 row = 0;

#define MAX_SCANCODE_NUM 128

static const char key_map[MAX_SCANCODE_NUM][2] = {
    /*00*/ {0x0, 0x0},   {0x0, 0x0}, {'1', '!'},   {'2', '@'},
    /*04*/ {'3', '#'},   {'4', '$'}, {'5', '%'},   {'6', '^'},
    /*08*/ {'7', '&'},   {'8', '*'}, {'9', '('},   {'0', ')'},
    /*0c*/ {'-', '_'},   {'=', '+'}, {'\b', '\b'}, {'\t', '\t'},
    /*10*/ {'q', 'Q'},   {'w', 'W'}, {'e', 'E'},   {'r', 'R'},
    /*14*/ {'t', 'T'},   {'y', 'Y'}, {'u', 'U'},   {'i', 'I'},
    /*18*/ {'o', 'O'},   {'p', 'P'}, {'[', '{'},   {']', '}'},
    /*1c*/ {'\n', '\n'}, {0x0, 0x0}, {'a', 'A'},   {'s', 'S'},
    /*20*/ {'d', 'D'},   {'f', 'F'}, {'g', 'G'},   {'h', 'H'},
    /*24*/ {'j', 'J'},   {'k', 'K'}, {'l', 'L'},   {';', ':'},
    /*28*/ {'\'', '\"'}, {'`', '~'}, {0x0, 0x0},   {'\\', '|'},
    /*2c*/ {'z', 'Z'},   {'x', 'X'}, {'c', 'C'},   {'v', 'V'},
    /*30*/ {'b', 'B'},   {'n', 'N'}, {'m', 'M'},   {',', '<'},
    /*34*/ {'.', '>'},   {'/', '?'}, {0x0, 0x0},   {'*', '*'},
    /*38*/ {0x0, 0x0},   {32, 32},   {0x0, 0x0},   {0x0, 0x0},
    /*3c*/ {0x0, 0x0},   {0x0, 0x0}, {0x0, 0x0},   {0x0, 0x0},
    /*40*/ {0x0, 0x0},   {0x0, 0x0}, {0x0, 0x0},   {0x0, 0x0},
    /*44*/ {0x0, 0x0},   {0x0, 0x0}, {0x0, 0x0},   {0x0, 0x0},
    /*48*/ {0x0, 0x0},   {0x0, 0x0}, {'-', '-'},   {0x0, 0x0},
    /*4c*/ {0x0, 0x0},
};

static const char key_map2[MAX_SCANCODE_NUM][2] = {
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {'`', '`'}, {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {'q', 'Q'}, {'1', '!'}, {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {'z', 'Z'}, {'s', 'S'}, {'a', 'A'},
    {'w', 'W'}, {'2', '@'}, {0, 0},     {0, 0},     {'c', 'C'}, {'x', 'X'},
    {'d', 'D'}, {'e', 'E'}, {'4', '$'}, {'3', '#'}, {0, 0},     {0, 0},
    {' ', ' '}, {'v', 'V'}, {'f', 'F'}, {'t', 'T'}, {'r', 'R'}, {'5', '%'},
    {0, 0},     {0, 0},     {'n', 'N'}, {'b', 'B'}, {'h', 'H'}, {'g', 'G'},
    {'y', 'Y'}, {'6', '^'}, {0, 0},     {0, 0},     {0, 0},     {'m', 'M'},
    {'j', 'J'}, {'u', 'U'}, {'7', '&'}, {'8', '*'}, {0, 0},     {0, 0},
    {',', '<'}, {'k', 'K'}, {'i', 'I'}, {'o', 'O'}, {'0', ')'}, {'9', '('},
    {0, 0},     {0, 0},     {'.', '>'}, {'/', '?'}, {'l', 'L'}, {';', ':'},
    {'p', 'P'}, {'-', '_'}, {0, 0},     {0, 0},     {0, 0},     {'\'', '"'},
    {0, 0},     {'[', '{'}, {'=', '+'}, {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},     {0, 0},
    {0, 0}};

int event_init() {
  if (event_info.is_init > 0) {
    return 0;
  }
  event_info.key_map_type=0;

  event_info.is_init = 1;
  event_info.input_fd = -1;
  event_info.joystick_fd = -1;
  event_info.mouse_fd = -1;

  printf("joystic init\n");
  event_info.joystick_fd = open("/dev/joystick", 0);
  if (event_info.joystick_fd < 0) {
    printf("open joystick failed\n");

    event_info.input_fd = open("/dev/keyboard", 0);
    if (event_info.input_fd < 0) {
      printf("open input failed\n");
    } else {
      dup2(event_info.input_fd, 0);
    }

  } else {
    dup2(event_info.joystick_fd, 0);
  }
  printf("joystic init end\n");

  event_info.mouse_fd = open("/dev/mouse", 0);
  if (event_info.mouse_fd < 0) {
    printf("open mouse failed\n");
  }
  printf("mouse init end2\n");

  return 1;
}

u32 scan_code_to_key(u32 scan_code) {
  int index = scan_code & 0x7f;
  if (index > MAX_SCANCODE_NUM) {
    printf("scan code overflow %x\n", scan_code);
    return 0;
  }
  if(event_info.key_map_type==0){
    return key_map[index][shf_p];
  }else {
    return key_map2[index][shf_p];
  }
  return key_map[index][shf_p];
}

int event_read_mouse(mouse_data_t* mouse_data) {
  if (event_info.mouse_fd < 0) return 0;
  int ret = read(event_info.mouse_fd, &event_info.mouse, sizeof(mouse_data_t));
  if (ret <= 0) return 0;
  *mouse_data = event_info.mouse;
  return 1;
}

int event_read_key(u32* key) {
  if (event_info.input_fd < 0) {
    return -1;
  }
  int ret = read(event_info.input_fd, &event_info.scan_code, 1);
  if (ret <= 0) return 0;
  *key = scan_code_to_key(event_info.scan_code);
  if (event_info.scan_code & 0x80) {
    return 1;
  } else {
    return 2;
  }
  return ret;
}

int event_read_joystick(u32* key) {
  if (event_info.joystick_fd < 0) {
    return -1;
  }
  int ret = read(event_info.joystick_fd, &event_info.joystick_code, 1);
  if (ret <= 0) return 0;

  *key = event_info.joystick_code & 0x7f;
  if (event_info.joystick_code & 0x80) {
    return 1;
  } else {
    return 2;
  }
  return ret;
}

int event_poll(event_t* event) {
  mouse_data_t mouse;
  event_t e;
  u32 count = 0;
  if (event_read_mouse(&mouse) > 0) {
    if (current_mouse.sate != mouse.sate) {
      u8 button = mouse.sate & BUTTON_MASK;
      if (button == BUTTON_LEFT || button == BUTTON_RIGHT ||
          button == BUTTON_MIDDLE) {
        if (button_press == 0) {
          e.type = MOUSE_BUTTON_DOWN;
          e.mouse = mouse;
          aqueue_push(&queue, e);
          button_press = 1;
        }
      } else {
        if (button_press == 1) {
          e.type = MOUSE_BUTTON_UP;
          e.mouse = mouse;
          aqueue_push(&queue, e);
          button_press = 0;
        }
      }
      count++;
      current_mouse = mouse;
    }
    if (current_mouse.x != mouse.x || current_mouse.y != mouse.y) {
      e.mouse = mouse;
      e.type = MOUSE_MOTION;
      aqueue_push(&queue, e);
      count++;
      current_mouse = mouse;
    }
  }

  u32 key;
  u32 press = event_read_key(&key);
  if (press > 0) {
    if (press == 1) {
      e.type = KEY_PRESS_DOWN;
    } else if (press == 2) {
      e.type = KEY_PRESS_UP;
    }
    e.key = key;
    aqueue_push(&queue, e);
    count++;
  }

  press = event_read_joystick(&key);
  if (press > 0) {
    if (press == 1) {
      e.type = KEY_PRESS_DOWN;
    } else if (press == 2) {
      e.type = KEY_PRESS_UP;
    }
    e.key = key;
    aqueue_push(&queue, e);
    count++;
  }

  if (aqueue_is_empty(&queue)) {
    return 0;
  }
  *event = aqueue_pop(&queue);
  return count;
}
