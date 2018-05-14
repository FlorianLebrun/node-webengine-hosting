#include "./WebxHttpTransaction.h"

// ------------------------------------------
// Transaction request

WebxHttpTransaction::WebxHttpTransaction(v8::Local<v8::Object> req, v8::Local<v8::Function> onComplete)
  : response(this)
{
  using namespace v8;
  this->opposite = 0;
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
}
WebxHttpTransaction::~WebxHttpTransaction()
{
}
webx::IStream *WebxHttpTransaction::getResponse()
{
  return &this->response;
}
void WebxHttpTransaction::setOpposite(webx::IStream *stream)
{
  this->opposite = stream;
}


// ------------------------------------------
// Transaction response

WebxHttpResponse::WebxHttpResponse(WebxHttpTransaction *transaction)
  : output(this, this->completeSync)
{
  this->transaction = transaction;
}

void WebxHttpResponse::setOpposite(IStream *stream)
{
  throw "not supported";
}
bool WebxHttpResponse::write(webx::IData *data)
{
  this->output.push_idle(data);
  return true;
}

void WebxHttpResponse::close()
{
  this->output.complete();
}


struct ResponseVisitor : public webx::StringAttributsVisitor<webx::IAttributsVisitor>
{
  int statusCode;
  v8::Local<v8::Object> headers;
  v8::Local<v8::Value> body;

  ResponseVisitor(WebxHttpResponse* response)
    : statusCode(0), headers(Nan::New<v8::Object>()), body(getBody(response))
  {
    response->visitAttributs(this);
  }
  virtual void visitString(const char *name, const char *value) override
  {
    if (name[0] == ':') this->statusCode = atoi(value);
    else this->headers->Set(Nan::New(name).ToLocalChecked(), Nan::New(value).ToLocalChecked());
  }

  static v8::Local<v8::Value> getBody(WebxHttpResponse* response)
  {
    using namespace v8;
    webx::IData *data = response->output.flush();
    if (data)
    {
      if (!data->next)
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
};

void WebxHttpResponse::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxHttpResponse *_this = (WebxHttpResponse *)handle->data;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ResponseVisitor response(_this);
  Local<Value> argv[] = {
    /*status*/ Nan::New(response.statusCode),
    /*headers*/ response.headers,
    /*buffer*/ response.body };
  Local<Function> onComplete = Local<Function>::New(isolate, _this->transaction->onComplete);
  onComplete->Call(isolate->GetCurrentContext()->Global(), 3, argv);

  _this->output.close(_this->closeSync);
}


void WebxHttpResponse::closeSync(uv_handle_t *handle) {
  WebxHttpResponse *_this = (WebxHttpResponse *)handle->data;
  WebxHttpTransaction *transaction = _this->transaction;
  _this->transaction = 0;
  transaction->persistent().Reset();
  delete transaction;
}