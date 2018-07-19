#ifndef WebxHttpTransaction_H
#define WebxHttpTransaction_H

#include "./v8helper.h"
#include "./WebxSession.h"

class WebxHttpTransaction;
class WebxHttpTransactionJS;

class WebxHttpResponse : public v8h::StringMapBasedAttributs<webx::IStream>
{
public:
  enum tState { Pending, Writing, Completed };

  tState state;
  WebxHttpTransaction *transaction;
  v8h::EventQueue<webx::IData> output;

  WebxHttpResponse(WebxHttpTransaction *transaction);

  virtual bool connect(webx::IStream *stream) override;
  virtual bool write(webx::IData *data) override;
  virtual void close() override;

  virtual void retain() override;
  virtual void release() override;

  static void completeSync(uv_async_t *handle);
};

class WebxHttpTransaction : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::Releasable<webx::IHttpTransaction>>
{
public:
  friend WebxHttpTransactionJS;
  webx::Ref<webx::IStream> opposite;
  WebxHttpResponse response;
  v8::Persistent<v8::Function> onBegin;
  v8::Persistent<v8::Function> onWrite;
  v8::Persistent<v8::Function> onEnd;

  WebxHttpTransaction(
    v8::Local<v8::Object> req,
    v8::Local<v8::Function> onBegin,
    v8::Local<v8::Function> onWrite,
    v8::Local<v8::Function> onEnd);
  ~WebxHttpTransaction();

  virtual bool connect(webx::IStream *stream) override;
  virtual webx::IStream *getResponse() override;
  virtual void free() override;
};

class WebxHttpTransactionJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);

  static void write(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
