#include "./WebxWebSocketStream.h"

WebxWebSocketStream::WebxWebSocketStream(v8::Local<v8::Object> req, v8::Local<v8::Function> onMessage, v8::Local<v8::Function> onClose)
{
  using namespace v8;
  this->opposite = 0;
  this->shallClose = 0;
  this->onMessage.Reset(Isolate::GetCurrent(), onMessage);
  this->onClose.Reset(Isolate::GetCurrent(), onClose);

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

  // Create async callback
  uv_loop_t *loop = uv_default_loop();
  this->async.data = this;
  uv_async_init(loop, &this->async, completeSync);
}
WebxWebSocketStream::~WebxWebSocketStream()
{
}

void WebxWebSocketStream::setOpposite(webx::IStream *stream) {
  this->opposite = stream;
  for (webx::IData* data = this->input_queue.flush(); data; data = data->next) {
    this->opposite->write(data);
  }
}

void WebxWebSocketStream::read(webx::IData* data) {
  data->from = this;
  if (this->opposite) {
    this->opposite->write(data);
  }
  else {
    this->input_queue.push(data);
  }
}
bool WebxWebSocketStream::write(webx::IData* data)
{
  this->output_lock.lock();
  this->output_queue.push(data);
  this->output_lock.unlock();
  uv_async_send(&this->async);
  return true;
}

void WebxWebSocketStream::close()
{
  this->shallClose = 1;
  uv_async_send(&this->async);
}

void WebxWebSocketStream::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxWebSocketStream *_this = (WebxWebSocketStream *)handle->data;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  webx::IData* datagrams;
  _this->output_lock.lock();
  datagrams = _this->output_queue.flush();
  _this->output_lock.unlock();
  if (datagrams) {
    for (webx::IData* data = datagrams; data; data = data->next) {
      Local<Value> argv[] = { v8h::MakeString(data->bytes, data->size) };
      Local<Function> onMessage = Local<Function>::New(isolate, _this->onMessage);
      onMessage->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    }
    datagrams->release();
  }

  if (_this->shallClose == 1) {
    printf("shallClose\n");
    Local<Value> argv[] = { Nan::New("close").ToLocalChecked() };
    Local<Function> onClose = Local<Function>::New(isolate, _this->onClose);
    onClose->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    _this->shallClose = 2;
  }
}
