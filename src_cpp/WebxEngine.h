#ifndef WebxEngine_H
#define WebxEngine_H

#include "./v8helper.h"
#include "./WebxSessionObjectWrap.h"
#include "./WebxSession.h"

class WebxEngine;
class WebxEngineJS;
class WebxSession;

class WebxEngine : public WebxSessionObjectWrap, public webx::NoAttributs<webx::Releasable<webx::IEngineHost>>
{
public:
  friend WebxEngineJS;
  webx::Ref<webx::IEngineContext> instance;

  WebxEngine(v8::Local<v8::Function> onEvent);
  ~WebxEngine();

  void connect(const char *dllPath, const char *dllEntryPoint, const char *config);
  virtual void dispatchTransaction(webx::IStream *request) override;
  virtual void dispatchWebSocket(webx::IStream *stream) override;
  virtual void notify(webx::IEvent* event) override;
  virtual bool disconnect() override;
  virtual void free() override;

  virtual void completeEvents() override;
};

class WebxEngineJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &args);

  static void getName(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
