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
  if (WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args.Holder())) {
    std::string name = engine->context->getName();
    args.GetReturnValue().Set(Nan::New(name).ToLocalChecked());
  }
  else {
    Nan::ThrowError("Cannot get a engine name on a closed engine");
  }
}

void WebxEngineJS::dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args) {
  Nan::ThrowError("WebxEngineJS::dispatchEvent not implemented");
}

void WebxEngineJS::consoleFlush(const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args.Holder())) {
    if (engine->instance) engine->instance->consoleFlush();
    fflush(stdout);
  }
}

void WebxEngineJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args.Holder())) {
    engine->context->close();
  }
  else {
    Nan::ThrowError("Cannot close on a closed engine");
  }
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
  Nan::SetPrototypeMethod(tpl, "consoleFlush", WebxEngineJS::consoleFlush);
  Nan::SetPrototypeMethod(tpl, "close", WebxEngineJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
