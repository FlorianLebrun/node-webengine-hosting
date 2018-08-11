#include "./WebxWebSocketStream.h"

Nan::Persistent<v8::Function> WebxWebSocketStreamJS::constructor;

void WebxWebSocketStreamJS::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 4 || !args[0]->IsObject() || !args[1]->IsObject() || !args[2]->IsFunction() || !args[3]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  WebxSession *session = WebxSession::Unwrap<WebxSession>(args[0]->ToObject());
  Local<Object> req = args[1]->ToObject();
  Local<Function> onAccept = args[2].As<v8::Function>();
  Local<Function> onReject = args[3].As<v8::Function>();

  WebxWebSocketStream *wsocket = new WebxWebSocketStream(req, onAccept, onReject);
  wsocket->AttachObject(args.This());

  session->context->dispatchWebSocket(wsocket);

  args.GetReturnValue().Set(args.This());
}

void WebxWebSocketStreamJS::on(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");

  WebxWebSocketStream *stream = WebxWebSocketStream::Unwrap<WebxWebSocketStream>(args.Holder());
  std::string event = v8h::GetUtf8(args[0]->ToString());
  Local<Function> callback = args[1].As<v8::Function>();
  if (event == "message")
  {
    stream->onMessage.Reset(Isolate::GetCurrent(), callback);
  }
  else if (event == "close")
  {
    stream->onClose.Reset(Isolate::GetCurrent(), callback);
  }
  else
  {
    Nan::ThrowError("Invalid event");
  }
}

void WebxWebSocketStreamJS::write(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxWebSocketStream *stream = WebxWebSocketStream::Unwrap<WebxWebSocketStream>(args.Holder());
  if (args.Length() != 1)
    Nan::ThrowTypeError("Wrong arguments");

  if (webx::IData *data = v8h::NewDataFromValue(args[0]))
  {
    stream->read(data);
  }
}

void WebxWebSocketStreamJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxWebSocketStream *stream = WebxWebSocketStream::Unwrap<WebxWebSocketStream>(args.Holder());
  if (stream->opposite)
    stream->opposite->close();
}

void WebxWebSocketStreamJS::abort(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxWebSocketStream *stream = WebxWebSocketStream::Unwrap<WebxWebSocketStream>(args.Holder());
  stream->abort();
}

v8::Local<v8::Function> WebxWebSocketStreamJS::CreatePrototype()
{
  using namespace v8;

  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(WebxWebSocketStreamJS::New);
  tpl->SetClassName(Nan::New("WebxWebSocketStream").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "on", WebxWebSocketStreamJS::on);
  Nan::SetPrototypeMethod(tpl, "write", WebxWebSocketStreamJS::write);
  Nan::SetPrototypeMethod(tpl, "close", WebxWebSocketStreamJS::close);
  Nan::SetPrototypeMethod(tpl, "abort", WebxWebSocketStreamJS::abort);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
