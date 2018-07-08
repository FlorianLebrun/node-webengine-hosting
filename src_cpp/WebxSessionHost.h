#ifndef WebxSession_H
#define WebxSession_H

#include "./v8helper.h"
#include "./WebxEngine.h"

class WebxSession;
class WebxSessionJS;
class WebxMainSessionJS;

class WebxSession : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::Releasable<webx::ISessionHost>>
{
public:
  friend WebxSessionJS;
  friend WebxMainSessionJS;

  webx::ISessionContext *context;
  v8h::EventQueue<webx::IEvent> notifications;
  v8::Persistent<v8::Function> onNotification;

  WebxSession(v8::Local<v8::Function> onNotification);
  ~WebxSession();

  virtual void notify(webx::IEvent* event) override;
  virtual void dispatchTransaction(webx::IHttpTransaction *transaction) override {}
  virtual void dispatchWebSocket(webx::IStream *stream) override {}
  virtual bool disconnect() override { throw "not imp"; }

  static void completeSync(uv_async_t *handle);
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
