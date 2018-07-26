#ifndef WebxSession_H
#define WebxSession_H

#include "./v8helper.h"
#include "./WebxEngine.h"

class WebxSession;
class WebxSessionJS;
class WebxMainSessionJS;
class WebxEngine;

class WebxSession : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::Releasable<webx::ISessionHost>>
{
public:
  friend WebxSessionJS;
  friend WebxMainSessionJS;

  WebxEngine* engine;
  webx::ISessionContext *context;
  v8h::EventQueue<webx::IEvent> events;
  v8::Persistent<v8::Function> onEvent;

  WebxSession(v8::Local<v8::Function> onEvent);
  ~WebxSession();

  virtual void notify(webx::IEvent* event) override;
  virtual void dispatchTransaction(webx::IStream *request) override {}
  virtual void dispatchWebSocket(webx::IStream *stream) override {}
  virtual bool disconnect() override { throw "not imp"; }
  virtual void free() override;

  void completeEvents();
  static void completeEvents_sync(uv_async_t *handle) {
    ((WebxSession*)handle->data)->completeEvents();
  }
};

class WebxSessionJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &onNotification);

  static void getName(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

class WebxMainSessionJS : WebxSessionJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &onNotification);
};

#endif
