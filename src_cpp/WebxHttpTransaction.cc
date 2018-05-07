#include "./WebxHttpTransaction.h"

void WebxHttpResponse::setOpposite(IStream *stream)
{
  throw "not supported";
}
bool WebxHttpResponse::write(webx::IData *data)
{
  this->body.push(data);
  return true;
}
void WebxHttpResponse::close()
{
}
v8::Local<v8::Object> WebxHttpResponse::NewHeaders()
{
  using namespace v8;
  struct Visitor : public webx::IAttributs::IVisitor
  {
    Local<Object> headers;
    Visitor()
    {
      this->headers = Nan::New<Object>();
    }
    virtual void visitString(const char *name, const char *value) override
    {
      this->headers->Set(Nan::New(name).ToLocalChecked(), Nan::New(value).ToLocalChecked());
    }
  };
  Visitor visitor;
  this->visitAttributs(&visitor);
  return visitor.headers;
}
v8::Local<v8::Value> WebxHttpResponse::NewBuffer()
{
  using namespace v8;
  webx::IData *data = this->body.pop();
  if (data)
  {
    if (!this->body.first)
    {
      v8::Local<v8::Value> buffer = Nan::CopyBuffer((char *)data->bytes, data->size).ToLocalChecked();
      data->release();
      return buffer;
    }
    else
    {
      throw "WebxHttpResponse::NewBuffer";
    }
  }
  return Nan::Undefined();
}

WebxHttpTransaction::WebxHttpTransaction(v8::Local<v8::Object> req, v8::Local<v8::Function> onComplete)
{
  using namespace v8;
  this->request_stream = 0;
  this->onComplete.Reset(Isolate::GetCurrent(), onComplete);

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
WebxHttpTransaction::~WebxHttpTransaction()
{
}

webx::IStream *WebxHttpTransaction::getResponse()
{
  return &this->response_stream;
}
void WebxHttpTransaction::setOpposite(webx::IStream *stream)
{
  this->request_stream = stream;
}
void WebxHttpTransaction::complete(int statusCode)
{
  this->statusCode = statusCode;
  uv_async_send(&this->async);
}
void WebxHttpTransaction::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxHttpTransaction *_this = (WebxHttpTransaction *)handle->data;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  Local<Value> argv[] = {/*status*/ Nan::New(_this->statusCode),
                         /*headers*/ _this->response_stream.NewHeaders(),
                         /*buffer*/ _this->response_stream.NewBuffer()};
  Local<Function> onComplete = Local<Function>::New(isolate, _this->onComplete);
  onComplete->Call(isolate->GetCurrentContext()->Global(), 3, argv);

  uv_close((uv_handle_t *)&_this->async, [](uv_handle_t *handle) {
    WebxHttpTransaction *_this = (WebxHttpTransaction *)handle->data;
    _this->persistent().Reset();
    delete _this;
  });
}
