#ifndef WebxWebSocketStream_H
#define WebxWebSocketStream_H

#include "./v8helper.h"
#include "./WebxSession.h"

class WebxWebSocketStream;
class WebxWebSocketStreamJS;

class WebxWebSocketStream : public v8h::ObjectWrap, public v8h::StringMapBasedAttributs<webx::Releasable<webx::IStream>>
{
public:
  friend WebxWebSocketStreamJS;

  enum { Starting = 0, Accepted, Rejected, Closed } status, prevStatus;
  v8::Persistent<v8::Function> onClose;

  // For inbound stream
  webx::Ref<webx::IStream> opposite;
  v8::Persistent<v8::Function> onAccept;
  v8::Persistent<v8::Function> onReject;

  // For outbound stream
  v8::Persistent<v8::Function> onMessage;
  v8h::EventQueue<webx::IData> output;

  WebxWebSocketStream(v8::Local<v8::Object> req, v8::Local<v8::Function> onAccept, v8::Local<v8::Function> onReject);
  ~WebxWebSocketStream();
  void abort() {}

  virtual bool connect(webx::IStream* stream) override;
  virtual bool write(webx::IData* data) override;
  virtual void close() override;
  virtual void free() override;

  void read(webx::IData* data);

  void completeEvents();
  static void completeEvents_sync(uv_async_t *handle) {
    ((WebxWebSocketStream*)handle->data)->completeEvents();
  }
};

class WebxWebSocketStreamJS
{
public:
  static Nan::Persistent<v8::Function> constructor;
  static v8::Local<v8::Function> CreatePrototype();
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);

  static void on(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void write(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void close(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void abort(const Nan::FunctionCallbackInfo<v8::Value> &args);
};

#endif
