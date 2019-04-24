#include "./WebxHttpTransaction.h"

TRACE_LEAK(static std::atomic<intptr_t> leakcount = 0);

struct ResponseData : public webx::IAttributsVisitor
{
  int statusCode;
  v8::Local<v8::Object> headers;
  v8::Local<v8::Value> buffer;

  ResponseData(webx::IDatagram* response, v8h::EventQueue<webx::IData>& responseData)
    : statusCode(0), headers(Nan::New<v8::Object>())
  {
    char* buffer;
    uint32_t size;
    response->visitAttributs(this);
    if (webx::Ref<webx::IData> data = responseData.flush())
    {
      if (data->getData(buffer, size)) {
        this->buffer = node::Buffer::Copy(v8::Isolate::GetCurrent(), buffer, size).ToLocalChecked();
      }
      if(data->next) {
        throw "Transfert-Encoding 'chunked' is not well supported.";
      }
    }
  }
  virtual void visit(const char *name, webx::AttributValue value) override
  {
    if (name[0] == ':') {
      if (!stricmp(name, ":status")) {
        this->statusCode = value.toInt();
      }
    }
    else {
      this->headers->Set(Nan::New(name).ToLocalChecked(), Nan::New(value.getStringPtr(), value.getStringLen()).ToLocalChecked());
    }
  }
};

WebxHttpTransaction::WebxHttpTransaction(
  v8::Local<v8::Object> req,
  v8::Local<v8::Function> onSend,
  v8::Local<v8::Function> onChunk,
  v8::Local<v8::Function> onEnd)
  : responseData(this, this->completeEvents_sync)
{
  using namespace v8;
  this->onSendCallback.Reset(Isolate::GetCurrent(), onSend);
  this->onChunkCallback.Reset(Isolate::GetCurrent(), onChunk);
  this->onEndCallback.Reset(Isolate::GetCurrent(), onEnd);

  // Set request pseudo headers
  this->setAttributStringV8(":method", v8h::GetIn(req, "method"));
  this->setAttributStringV8(":path", v8h::GetIn(req, "url"));
  this->setAttributStringV8(":scheme", v8h::GetIn(req, "protocole"));
  this->setAttributStringV8(":authority", v8h::GetIn(req, "hostname"));
  this->setAttributStringV8(":original-path", v8h::GetIn(req, "originalUrl"));

  // Set request headers
  Local<Object> headers = v8h::GetIn(req, "headers")->ToObject();
  Local<Array> keys = headers->GetOwnPropertyNames(Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
  for (uint32_t i = 0; i < keys->Length(); i++)
  {
    Local<Value> key = keys->Get(i);
    this->setAttributStringV8(key, headers->Get(key));
  }
  TRACE_LEAK(printf("<WebxHttpTransaction %d>\n", int(++leakcount)));
}

WebxHttpTransaction::~WebxHttpTransaction()
{
  if (this->requestHandler) {
    this->requestHandler->disconnect();
    this->requestHandler = 0;
  }
  this->onSendCallback.Reset();
  this->onChunkCallback.Reset();
  this->onEndCallback.Reset();
  memset(this, 0, sizeof(void*)); // Force VMT clean (for better crash)
  TRACE_LEAK(printf("<WebxHttpTransaction %d>\n", int(--leakcount)));
}

bool WebxHttpTransaction::accept(webx::IDatagramHandler *handler)
{
  this->requestHandler = handler;
  return true;
}

bool WebxHttpTransaction::send(webx::IDatagram* response)
{
  if (!this->response && response->accept(this)) {
    this->response = response;
    return true;
  }
  return false;
}

void WebxHttpTransaction::onData(webx::IDatagram* from)
{
  this->responseData.push_idle(from->pullData());
}

void WebxHttpTransaction::onComplete(webx::IDatagram* from)
{
  this->responseData.complete();
}

void WebxHttpTransaction::disconnect()
{
  this->response = 0;
}

void WebxHttpTransaction::completeEvents() {
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  // Write the response stream
  Local<Function> onSend = Local<Function>::New(isolate, this->onSendCallback);
  ResponseData response(this->response, this->responseData);
  Local<Value> argv[] = {
    /*status*/ Nan::New(response.statusCode),
    /*headers*/ response.headers,
    /*buffer*/ response.buffer,
  };
  onSend->Call(isolate->GetCurrentContext()->Global(), 3, argv);

  // Close the response stream
  if (this->responseData.is_completed()) {
    Local<Function> onEnd = Local<Function>::New(isolate, this->onEndCallback);
    onEnd->Call(isolate->GetCurrentContext()->Global(), 0, 0);
    this->DettachObject();
    this->release();
  }
}

void WebxHttpTransaction::free()
{
  _ASSERT(!this->responseData.flush());
  //delete this; // Crash with vs2012 debugger
}
