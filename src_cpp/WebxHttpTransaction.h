#ifndef WebxHttpTransaction_H
#define WebxHttpTransaction_H

#include "./v8helper.h"
#include "./WebxEngineHost.h"

class WebxHttpResponse : public v8h::StringMapBasedAttributs<webx::IStream>
{
public:
  webx::DataQueue body;

  virtual void setOpposite(webx::IStream *stream) override;
  virtual bool write(webx::IData *data) override;
  virtual void close() override;

  v8::Local<v8::Object> NewHeaders();
  v8::Local<v8::Value> NewBuffer();
};

class WebxHttpTransaction : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::IHttpTransaction>
{
public:
  int statusCode;
  webx::IStream* request_stream;
  WebxHttpResponse response_stream;
  v8::Persistent<v8::Function> onComplete;
  uv_async_t async;
  
  WebxHttpTransaction(v8::Local<v8::Object> req, v8::Local<v8::Function> onComplete);
  ~WebxHttpTransaction();
  void abort() {}

  virtual void setOpposite(webx::IStream* stream) override;
  virtual webx::IStream *getResponse() override;
  virtual void complete(int statusCode) override;
  static void completeSync(uv_async_t *handle);

  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);
};

class WebxHttpTransactionJS
{
public:
  static v8::Local<v8::Function> CreatePrototype();
  static Nan::Persistent<v8::Function> constructor;

  static void write(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void abort(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
