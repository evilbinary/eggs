#include "etk_main_loop.h"

#include "etk_global.h"
#include <unistd.h>

EtkMainLoop* etk_main_loop_create(EtkSourcesManager* sources_manager) {
  EtkMainLoop* thiz = (EtkMainLoop*)ETK_MALLOC(sizeof(EtkMainLoop));
  if (thiz != NULL) {
    thiz->running = 0;
    thiz->sources_manager = sources_manager;
  }
  return thiz;
}
Ret etk_main_loop_run(EtkMainLoop* thiz) {
  e32 wait_time = 16;
  e32 source_wait_time = 0;
  EtkSource* source;
  static int boot_flush = 1;
  thiz->running = 1;

  while (thiz->running) {
    int need_flush = boot_flush;
    wait_time = 16;
    if (etk_event_dispatch() == RET_OK) {
      need_flush = 1;
    }
    for (source = thiz->sources_manager->sources; source != NULL;
         source = source->next) {
      source_wait_time = etk_source_check(source);
      if (source_wait_time >= 0 && source_wait_time < wait_time) {
        wait_time = source_wait_time;
      }

      if (source->disable > 0) {
        etk_sources_manager_remove(thiz->sources_manager, source);
        dbg_printf("etk_sources_manager_remove11\n");
        continue;
      }
      if ((source_wait_time = etk_source_check(source)) == 0) {
        need_flush = 1;
        if (etk_source_dispatch(source) != RET_OK) {
          dbg_printf("etk_sources_manager_remove33\n");
          etk_sources_manager_remove(thiz->sources_manager, source);
        }
      }
    }
    /* 空闲不 present：避免每帧 memcpy 到 FB 造成闪屏 */
    if (need_flush) {
      etk_display_flush();
      boot_flush = 0;
    }
    if (wait_time < 1) {
      wait_time = 1;
    }
    if (wait_time > 16) {
      wait_time = 16;
    }
    usleep((unsigned)wait_time * 1000u);
  }
}

EtkMainLoop* etk_default_main_loop(void) { return etkglobal.main_loop; }

void etk_main_loop_destroy(EtkMainLoop* thiz) {
  if (thiz != NULL) {
    ETK_FREE(thiz);
  }
}

Ret etk_main_loop_quit(EtkMainLoop* thiz) {
  EtkEvent event;
  etk_event_init(&event, ETK_EVENT_QUIT);
  thiz->running = 0;
  etk_event_queue(&event);
  return etk_source_queue_event(etk_default_source(), &event);
}
Ret etk_main_loop_add_source(EtkMainLoop* thiz, EtkSource* source) {
  EtkEvent event;
  etk_event_init(&event, ETK_EVENT_ADD_SOURCE);
  event.u.extra = source;
  etk_source_enable(source);
  return etk_source_queue_event(etk_default_source(), &event);
}
Ret etk_main_loop_remove_source(EtkMainLoop* thiz, EtkSource* source) {
  EtkEvent event;
  etk_event_init(&event, ETK_EVENT_REMOVE_SOURCE);
  event.u.extra = source;
  etk_source_enable(source);
  return etk_source_queue_event(etk_default_source(), &event);
}
