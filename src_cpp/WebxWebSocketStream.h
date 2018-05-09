#ifndef WebxWebSocketStream_H
#define WebxWebSocketStream_H

#include "./v8helper.h"
#include "./WebxEngineHost.h"

class WebxWebSocketStream : public Nan::ObjectWrap, public v8h::StringMapBasedAttributs<webx::IStream>
{
public:

  enum { Nothing = 0, Accepted, Rejected, Closed } status, prevStatus;
  int isClosed;
  v8::Persistent<v8::Function> onClose;

  // For inbound stream
  webx::IStream* opposite;
  v8::Persistent<v8::Function> onAccept;
  v8::Persistent<v8::Function> onReject;

  // For outbound stream
  v8::Persistent<v8::Function> onMessage;
  webx::DataQueue output_queue;
  SpinLock output_lock;
  uv_async_t async;

  WebxWebSocketStream(v8::Local<v8::Object> req, v8::Local<v8::Function> onAccept, v8::Local<v8::Function> onReject);
  ~WebxWebSocketStream();
  void abort() {}

  virtual void setOpposite(webx::IStream* stream) override;
  virtual bool write(webx::IData* data) override;
  virtual void close() override;

  void read(webx::IData* data);

  static void completeSync(uv_async_t *handle);

  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);
};

class WebxWebSocketStreamJS
{
public:
  static v8::Local<v8::Function> CreatePrototype();
  static Nan::Persistent<v8::Function> constructor;

  static void on(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void write(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void abort(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif