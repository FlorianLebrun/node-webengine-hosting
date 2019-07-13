#ifndef WebxEngine_H
#define WebxEngine_H

#include "./v8helper.h"
#include "./WebxSessionObjectWrap.h"
#include "./WebxSession.h"

class WebxEngine;
class WebxEngineJS;
class WebxSession;

class WebxEngine : public WebxSessionObjectWrap, public webx::Releasable<webx::IEngineHost, WebxEngine>
{
public:
  friend WebxEngineJS;
  webx::Ref<webx::IEngine> instance;

  WebxEngine(v8::Local<v8::Function> onEvent);
  virtual ~WebxEngine() override;

  void connect(const char *dllPath, const char *dllEntryPoint, const char *config);
  virtual void dispatchDatagram(webx::IDatagram *datagram) override;
  virtual void dispatchEvent(webx::IEvent* event) override;
  virtual bool disconnect(webx::ISession* session) override;
  virtual bool terminate() override;

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
  static void consoleFlush(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
