#ifndef WebxSession_H
#define WebxSession_H

#include "./v8helper.h"
#include "./WebxSessionObjectWrap.h"
#include "./WebxEngine.h"

class WebxSession;
class WebxSessionJS;
class WebxEngine;

class WebxSession : public WebxSessionObjectWrap, public webx::Releasable<webx::ISessionHost, WebxSession>
{
public:
  friend WebxSessionJS;

  WebxEngine* engine;

  WebxSession(v8::Local<v8::Function> onEvent);
  ~WebxSession();

  virtual void dispatchEvent(webx::IEvent* cevent) override;
  virtual void dispatchDatagram(webx::IDatagram *datagram) override;
  virtual bool disconnect(webx::ISession* session) override;
  virtual void free() override;

  virtual void completeEvents() override;
};

class WebxSessionJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &onNotification);

  static void getName(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
