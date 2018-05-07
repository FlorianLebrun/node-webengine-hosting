#ifndef WebxEngineHost_H
#define WebxEngineHost_H

#include "./v8helper.h"
#include "./WebxHttpTransaction.h"


class WebxEngineHost : public Nan::ObjectWrap, public webx::IEngineHost
{
public:
  webx::IEngineConnector *connector;
  /*uv_async_t queue_async;
  uv_mutex_t queue_mutex;
  webx::DataQueue queue_datas;*/

  
  WebxEngineHost();
  ~WebxEngineHost();

  void connect(const char *dllPath, const char *dllEntryPoint, const char *config);

  virtual void dispatchTransaction(webx::IHttpTransaction *transaction) override {}
  virtual void dispatchWebSocket(webx::IStream *stream) override {}
  virtual void notify(const char *event, const void *bytes, int32_t length) override;

  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);
  static void completeSync(uv_async_t *handle);
};

class WebxEngineHostJS
{
public:
	static v8::Local<v8::Function> CreatePrototype();
	static Nan::Persistent<v8::Function> constructor;

  static void connect(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void getName(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
