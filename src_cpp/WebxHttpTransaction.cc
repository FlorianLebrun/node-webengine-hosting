#include "./WebxHttpTransaction.h"

struct ResponseData 
{
  int statusCode;
  v8::Local<v8::Object> headers;
  v8::Local<v8::Value> buffer;

  ResponseData(webx::IDatagram* response)
    : statusCode(0), headers(Nan::New<v8::Object>())
  {
    char* buffer;
    uint32_t size;
    if (webx::IValue* responseAttributs = response->getAttributs()) {
      responseAttributs->foreach([this](const webx::IValue& key, const webx::IValue& value) {
        std::string name = key.toString();
        if (name[0] == ':') {
          if (!stricmp(name.c_str(), ":status")) {
            this->statusCode = value.toInteger();
          }
        }
        else {
          this->headers->Set(Nan::New(name).ToLocalChecked(), Nan::New(value.toString()).ToLocalChecked());
        }
      });
    }

    webx::Ref<webx::IData> data;
    data.New(response->pullData());
    if (data)
    {
      if (data->getData(buffer, size)) {
        this->buffer = node::Buffer::Copy(v8::Isolate::GetCurrent(), buffer, size).ToLocalChecked();
      }
      if(data->next) {
        throw "Transfert-Encoding 'chunked' is not well supported.";
      }
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
  this->requestAttributs.values[":method"] = v8h::GetUtf8(v8h::GetIn(req, "method"));
  this->requestAttributs.values[":path"] = v8h::GetUtf8(v8h::GetIn(req, "url"));
  this->requestAttributs.values[":scheme"] = v8h::GetUtf8(v8h::GetIn(req, "protocole"));
  this->requestAttributs.values[":authority"] = v8h::GetUtf8(v8h::GetIn(req, "hostname"));
  this->requestAttributs.values[":original-path"] = v8h::GetUtf8(v8h::GetIn(req, "originalUrl"));

  // Set request headers
  Local<Object> headers = v8h::GetIn(req, "headers")->ToObject();
  Local<Array> keys = headers->GetOwnPropertyNames(Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
  for (uint32_t i = 0; i < keys->Length(); i++)
  {
    Local<Value> key = keys->Get(i);
    this->requestAttributs.values[v8h::GetUtf8(key)] = v8h::GetUtf8(headers->Get(key));
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

void WebxHttpTransaction::discard()
{
  throw;
}

bool WebxHttpTransaction::send(webx::IDatagram* response)
{
  if (!this->response) {
    this->response = response;
    return response->accept(this);
  }
  return false;
}

webx::IData* WebxHttpTransaction::pullData() {
  return this->requestData.flush();
}

void WebxHttpTransaction::onData(webx::IDatagram* from)
{
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
  if (this->response) {
    ResponseData response(this->response);
    Local<Value> argv[] = {
      /*status*/ Nan::New(response.statusCode),
      /*headers*/ response.headers,
      /*buffer*/ response.buffer,
    };
    onSend->Call(isolate->GetCurrentContext()->Global(), 3, argv);
  }
  else {
    Local<Value> argv[] = {
      /*status*/ Nan::New(404),
      /*headers*/ Nan::New<v8::Object>(),
    };
    onSend->Call(isolate->GetCurrentContext()->Global(), 2, argv);
  }

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
