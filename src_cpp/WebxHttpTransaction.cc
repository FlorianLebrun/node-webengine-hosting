#include "./WebxHttpTransaction.h"

static std::atomic<intptr_t> leakcount = 0;

struct ResponseData : public webx::StringAttributsVisitor<webx::IAttributsVisitor>
{
  int statusCode;
  v8::Local<v8::Object> headers;
  v8::Local<v8::Value> buffer;

  ResponseData(webx::IData* data)
    : statusCode(0), headers(Nan::New<v8::Object>())
  {
    char* buffer;
    uint32_t size;
    data->visitAttributs(this);
    if (data->getData(buffer, size)) {
      this->buffer = node::Buffer::Copy(v8::Isolate::GetCurrent(), buffer, size).ToLocalChecked();
    }
    else {
      throw "Transfert-Encoding 'chunked' is not well supported.";
    }
  }
  virtual void visitString(const char *name, const char *value) override
  {
    if (name[0] == ':') {
      if (!stricmp(name, ":status")) {
        this->statusCode = atoi(value);
      }
    }
    else this->headers->Set(Nan::New(name).ToLocalChecked(), Nan::New(value).ToLocalChecked());
  }
};

WebxHttpTransaction::WebxHttpTransaction(
  v8::Local<v8::Object> req,
  v8::Local<v8::Function> onSend,
  v8::Local<v8::Function> onChunk,
  v8::Local<v8::Function> onEnd)
  : output(this, this->completeEvents_sync)
{
  using namespace v8;
  this->onSend.Reset(Isolate::GetCurrent(), onSend);
  this->onChunk.Reset(Isolate::GetCurrent(), onChunk);
  this->onEnd.Reset(Isolate::GetCurrent(), onEnd);

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
  TRACE_LEAK(printf("<WebxHttpTransaction %d>\n", int(--leakcount)));
}

bool WebxHttpTransaction::connect(webx::IStream *stream)
{
  this->input = stream;
  return true;
}

bool WebxHttpTransaction::write(webx::IData *data)
{
  this->output.push_idle(data);
  return true;
}

void WebxHttpTransaction::close()
{
  this->output.complete();
}

void WebxHttpTransaction::completeEvents() {
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  // Write the response stream
  if (webx::Ref<webx::IData> output = this->output.flush())
  {
    Local<Function> onSend = Local<Function>::New(isolate, this->onSend);
    for (webx::IData* data = output; data; data = data->next.cast<webx::IData>())
    {
      ResponseData response(data);
      Local<Value> argv[] = {
        /*status*/ Nan::New(response.statusCode),
        /*headers*/ response.headers,
        /*buffer*/ response.buffer,
      };
      onSend->Call(isolate->GetCurrentContext()->Global(), 3, argv);
    }
  }

  // Close the response stream
  if (this->output.is_completed()) {
    Local<Function> onEnd = Local<Function>::New(isolate, this->onEnd);
    onEnd->Call(isolate->GetCurrentContext()->Global(), 0, 0);
    this->DettachObject();
    this->release();
  }
}

void WebxHttpTransaction::free()
{
  _ASSERT(!this->output.flush());
  delete this;
}
