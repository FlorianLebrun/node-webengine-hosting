#include "./WebxWebSocketStream.h"

WebxWebSocketStream::WebxWebSocketStream(v8::Local<v8::Object> req, v8::Local<v8::Function> onAccept, v8::Local<v8::Function> onReject)
    : output(this, this->completeEvents_sync)
{
  using namespace v8;
  this->status = Starting;
  this->prevStatus = Starting;
  this->onAccept.Reset(Isolate::GetCurrent(), onAccept);
  this->onReject.Reset(Isolate::GetCurrent(), onReject);

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
}

WebxWebSocketStream::~WebxWebSocketStream()
{
}

bool WebxWebSocketStream::connect(webx::IStream *stream)
{
  this->opposite = stream;
  this->status = Accepted;
  this->output.complete();
  return true;
}

void WebxWebSocketStream::read(webx::IData *data)
{
  if (this->opposite)
  {
    this->opposite->write(data);
  }
  else
  {
    throw "Shall be accepted before data";
  }
}
bool WebxWebSocketStream::write(webx::IData *data)
{
  this->output.push(data);
  return true;
}

void WebxWebSocketStream::close()
{
  if (this->status == Starting)
    this->status = Rejected;
  else
    this->status = Closed;
  this->output.complete();
}

void WebxWebSocketStream::free() {
  printf("WebxWebSocketStream leak !\n");
}

void WebxWebSocketStream::completeEvents()
{
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (!this->onMessage.IsEmpty())
  {
    if (webx::Ref<webx::IData> datagrams = this->output.flush())
    {
      for (webx::IData *data = datagrams; data; data = data->next.cast<webx::IData>())
      {
        char* buffer;
        uint32_t size;
        data->getData(buffer, size);
        Local<Value> argv[] = {v8h::MakeString(buffer, size)};
        Local<Function> onMessage = Local<Function>::New(isolate, this->onMessage);
        onMessage->Call(isolate->GetCurrentContext()->Global(), 1, argv);
      }
    }
  }

  if (this->status != this->prevStatus)
  {
    switch (this->status)
    {
    case Accepted:
    {
      Local<Value> argv[] = {this->persistent().Get(isolate)};
      Local<Function> onAccept = Local<Function>::New(isolate, this->onAccept);
      onAccept->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    }
    break;
    case Rejected:
    {
      Local<Value> argv[] = {Nan::New(404)};
      Local<Function> onReject = Local<Function>::New(isolate, this->onReject);
      onReject->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    }
    break;
    case Closed:
    {
      Local<Function> onClose = Local<Function>::New(isolate, this->onClose);
      onClose->Call(isolate->GetCurrentContext()->Global(), 0, 0);
    }
    break;
    }
    this->prevStatus = this->status;
  }
}
