#include "./WebxSession.h"

Nan::Persistent<v8::Function> WebxSessionJS::constructor;
Nan::Persistent<v8::Function> WebxMainSessionJS::constructor;

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

v8::Local<v8::Function> WebxSessionJS::CreatePrototype()
{

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxSessionJS::New);
  tpl->SetClassName(Nan::New("WebxSession").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getName", WebxSessionJS::getName);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}

void WebxMainSessionJS::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 3 || !args[0]->IsObject() || !args[1]->IsString() || !args[2]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  WebxEngine *engine = Nan::ObjectWrap::Unwrap<WebxEngine>(args[0]->ToObject());
  Local<String> config = args[1].As<v8::String>();

  if (!engine->mainSession) {
    Isolate *isolate = Isolate::GetCurrent();
    WebxSession *session = new WebxSession(args[2].As<v8::Function>());
    session->engine = engine;
    session->context = engine->context->createMainSession(session, *Utf8Value(config));
    session->Wrap(args.This());

    engine->mainSession = session;
    engine->handle()->Set(String::NewFromUtf8(isolate, "mainSession"), args.This());
    args.GetReturnValue().Set(args.This());
  }
  else {
    Nan::ThrowError("Main session already exists");
  }
}

v8::Local<v8::Function> WebxMainSessionJS::CreatePrototype()
{

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxMainSessionJS::New);
  tpl->SetClassName(Nan::New("WebxMainSession").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getName", WebxSessionJS::getName);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
