#include "./WebxSession.h"

Nan::Persistent<v8::Function> WebxSessionJS::constructor;

typedef v8::String::Utf8Value Utf8Value;

void WebxSessionJS::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 4 || !args[0]->IsObject() || !args[1]->IsString() || !args[2]->IsString() || !args[3]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  WebxEngine *engine = Nan::ObjectWrap::Unwrap<WebxEngine>(args[0]->ToObject());
  Local<String> name = args[1].As<v8::String>();
  Local<String> config = args[2].As<v8::String>();

  WebxSession *session = new WebxSession(args[3].As<v8::Function>());
  session->context = engine->context->createSession(session, *Utf8Value(name), *Utf8Value(config));
  session->Wrap(args.This());

  args.GetReturnValue().Set(args.This());
}

void WebxSessionJS::getName(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxSession *session = Nan::ObjectWrap::Unwrap<WebxSession>(args.Holder());
  const char *name = session->context->getName();
  args.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), name));
}

void WebxSessionJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxSession *session = Nan::ObjectWrap::Unwrap<WebxSession>(args.Holder());
  session->context->close();
}

v8::Local<v8::Function> WebxSessionJS::CreatePrototype()
{

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxSessionJS::New);
  tpl->SetClassName(Nan::New("WebxSession").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getName", WebxSessionJS::getName);
  Nan::SetPrototypeMethod(tpl, "close", WebxSessionJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
