#ifndef WebxEngine_H
#define WebxEngine_H

#include "./v8helper.h"
#include "./WebxSession.h"

class WebxEngine;
class WebxEngineJS;
class WebxSession;

class WebxEngine : public Nan::ObjectWrap, public webx::Releasable<webx::IEngineHost>
{
public:
  friend WebxEngineJS;

  webx::Ref<webx::IEngineContext> context;
  webx::Ref<WebxSession> mainSession;

  v8h::EventQueue<webx::IEvent> events;
  v8::Persistent<v8::Function> handleEvent;

  WebxEngine(v8::Local<v8::Function> handleEvent);
  ~WebxEngine();

  void connect(const char *dllPath, const char *dllEntryPoint, const char *config);
  virtual void notify(webx::IEvent* event) override;
  virtual bool disconnect() override { throw "not imp"; }
  virtual void free() override { delete this; }

  static void completeSync(uv_async_t *handle);
};

class WebxEngineJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &args);

  static void getName(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
