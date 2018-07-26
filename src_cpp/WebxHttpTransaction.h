#ifndef WebxHttpTransaction_H
#define WebxHttpTransaction_H

#include "./v8helper.h"
#include "./WebxSession.h"

class WebxHttpTransaction;
class WebxHttpTransactionJS;

class WebxHttpTransaction : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::Releasable<webx::IStream>>
{
public:
  friend WebxHttpTransactionJS;

  webx::Ref<webx::IStream> input;
  v8h::EventQueue<webx::IData> output;
  v8::Persistent<v8::Function> onSend;
  v8::Persistent<v8::Function> onChunk;
  v8::Persistent<v8::Function> onEnd;

  WebxHttpTransaction(
    v8::Local<v8::Object> req,
    v8::Local<v8::Function> onSend,
    v8::Local<v8::Function> onChunk,
    v8::Local<v8::Function> onEnd);
  ~WebxHttpTransaction();

  virtual bool connect(webx::IStream *stream) override;
  virtual bool write(webx::IData *data) override;
  virtual void close() override;
  virtual void free() override;

  void completeEvents();
  static void completeEvents_sync(uv_async_t *handle) {
    ((WebxHttpTransaction*)handle->data)->completeEvents();
  }
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
