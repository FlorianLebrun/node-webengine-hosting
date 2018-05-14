#ifndef WebxHttpTransaction_H
#define WebxHttpTransaction_H

#include "./v8helper.h"
#include "./WebxEngineHost.h"

class WebxHttpTransaction;
class WebxHttpTransactionJS;

class WebxHttpResponse : public v8h::StringMapBasedAttributs<webx::IStream>
{
public:
  WebxHttpTransaction *transaction;
  v8h::EventQueue<webx::IData> output;

  WebxHttpResponse(WebxHttpTransaction *transaction);

  v8::Local<v8::Value> getBody();

  virtual void setOpposite(webx::IStream *stream) override;
  virtual bool write(webx::IData *data) override;
  virtual void close() override;

  static void completeSync(uv_async_t *handle);
  static void closeSync(uv_handle_t *handle);
};

class WebxHttpTransaction : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::IHttpTransaction>
{
public:
  webx::IStream* opposite;
  WebxHttpResponse response;
  v8::Persistent<v8::Function> onComplete;
  
  WebxHttpTransaction(v8::Local<v8::Object> req, v8::Local<v8::Function> onComplete);
  ~WebxHttpTransaction();
  
  virtual void setOpposite(webx::IStream* stream) override;
  virtual webx::IStream *getResponse() override;

  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);
};

class WebxHttpTransactionJS
{
public:
  static v8::Local<v8::Function> CreatePrototype();
  static Nan::Persistent<v8::Function> constructor;

  static void write(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
