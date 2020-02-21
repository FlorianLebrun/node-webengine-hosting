#ifndef WebxSessionObjectWrap_H
#define WebxSessionObjectWrap_H

#include "./v8helper.h"

class WebxSessionObjectWrap : public v8h::ObjectWrap {
public:
  webx::Ref<webx::ISession> context;
  v8h::EventQueue<webx::IEvent> events;
  v8::Persistent<v8::Function> onEvent;

  WebxSessionObjectWrap(v8::Local<v8::Function> onEvent);
  ~WebxSessionObjectWrap();

  virtual void completeEvents() = 0;

  static void completeEvents_sync(uv_async_t *handle) {
    ((WebxSessionObjectWrap*)handle->data)->completeEvents();
  }
};

#endif
