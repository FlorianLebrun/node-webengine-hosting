#ifndef WebxHttpTransaction_H
#define WebxHttpTransaction_H

#include "./v8helper.h"
#include "./WebxSession.h"

class WebxHttpTransaction;
class WebxHttpTransactionJS;

class WebxHttpTransaction : public v8h::ObjectWrap, 
  public webx::Releasable<webx::IDatagram>, 
  public webx::IDatagramHandler
{
public:
  friend WebxHttpTransactionJS;

  webx::IDatagramHandler* requestHandler;
  webx::StringMapValue requestAttributs;
  webx::DataQueue requestData;

  webx::Ref<webx::IDatagram> response;
  v8h::EventQueue<webx::IData> responseData;

  v8::Persistent<v8::Function> onSendCallback;
  v8::Persistent<v8::Function> onChunkCallback;
  v8::Persistent<v8::Function> onEndCallback;
  webx::tIOStatus requestStatus;

  WebxHttpTransaction(
    v8::Local<v8::Object> req,
    v8::Local<v8::Function> onSend,
    v8::Local<v8::Function> onChunk,
    v8::Local<v8::Function> onEnd);
  ~WebxHttpTransaction();

  virtual bool accept(webx::IDatagramHandler *handler) override;
  virtual void discard() override;

  virtual webx::IValue* getAttributs() { return &this->requestAttributs; }

  virtual bool send(webx::IDatagram *response) override;
  virtual webx::tIOStatus getStatus() override {return this->requestStatus;}
  virtual webx::IData* pullData() override { return 0; }
  virtual webx::IDatagram* pullAttachment() override { return 0; }
  virtual void free() override;

  virtual void onData(webx::IDatagram* from);
  virtual void onAttachment(webx::IDatagram* from) {}
  virtual void onComplete(webx::IDatagram* from);
  virtual void disconnect();

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
