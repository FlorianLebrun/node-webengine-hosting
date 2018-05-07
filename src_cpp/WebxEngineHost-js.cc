#include "WebxEngineHost.h"

Nan::Persistent<v8::Function> WebxEngineHostJS::constructor;

void WebxEngineHost::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  WebxEngineHost *obj = new WebxEngineHost();
  obj->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
}

void WebxEngineHostJS::connect(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxEngineHost *host = Nan::ObjectWrap::Unwrap<WebxEngineHost>(args.Holder());

  if (host->connector)
    Nan::ThrowError("Already connected");
  if (args.Length() != 3 || !args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString())
    Nan::ThrowTypeError("Wrong arguments");

  typedef v8::String::Utf8Value Utf8Value;
  Local<String> arg_dllPath = args[0]->ToString();
  Local<String> arg_dllEntry = args[1]->ToString();
  Local<String> arg_config = args[2]->ToString();
  host->connect(*Utf8Value(arg_dllPath), *Utf8Value(arg_dllEntry), *Utf8Value(arg_config));
}

void WebxEngineHostJS::getName(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxEngineHost *host = Nan::ObjectWrap::Unwrap<WebxEngineHost>(args.Holder());
  const char *name = host->connector->getName();
  args.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), name));
}

v8::Local<v8::Function> WebxEngineHostJS::CreatePrototype() {

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxEngineHost::New);
  tpl->SetClassName(Nan::New("WebxEngineHost").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "connect", WebxEngineHostJS::connect);
  Nan::SetPrototypeMethod(tpl, "getName", WebxEngineHostJS::getName);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
