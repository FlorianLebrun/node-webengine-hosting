#include "./WebxEngine.h"
#include "./WebxSession.h"

Nan::Persistent<v8::Function> WebxEngineJS::constructor;

typedef v8::String::Utf8Value Utf8Value;

void WebxEngineJS::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 4 || !args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString() || !args[3]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  Local<String> dllPath = args[0]->ToString();
  Local<String> dllEntry = args[1]->ToString();
  Local<String> config = args[2]->ToString();

  WebxEngine *engine = new WebxEngine(args[3].As<v8::Function>());
  engine->AttachObject(args.This());
  engine->connect(*Utf8Value(dllPath), *Utf8Value(dllEntry), *Utf8Value(config));

  args.GetReturnValue().Set(args.This());
}

void WebxEngineJS::getName(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args.Holder());
  const char *name = engine->context->getName();
  args.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(), name));
}

void WebxEngineJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args.Holder());
  engine->context->close();
}

void WebxEngineJS::dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args) {

}

v8::Local<v8::Function> WebxEngineJS::CreatePrototype()
{
  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxEngineJS::New);
  tpl->SetClassName(Nan::New("WebxEngine").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getName", WebxEngineJS::getName);
  Nan::SetPrototypeMethod(tpl, "dispatchEvent", WebxEngineJS::dispatchEvent);
  Nan::SetPrototypeMethod(tpl, "close", WebxEngineJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
