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

  if (WebxEngine *engine = WebxEngine::Unwrap<WebxEngine>(args[0]->ToObject())) {
    std::string type = *Utf8Value(args[1].As<v8::String>());
    std::string name = *Utf8Value(args[2].As<v8::String>());

    WebxSession *session = new WebxSession(args[3].As<v8::Function>());
    if (webx::ISession* context = engine->instance->createSession(type.c_str(), name.c_str(), session)) {
      session->context.Box(context);
      session->AttachObject(args.This());
      args.GetReturnValue().Set(args.This());
    }
    else {
      delete session;
      Nan::ThrowError("Cannot create this type of session");
    }
  }
  else {
    Nan::ThrowError("Cannot create a session on a closed engine");
  }
}

void WebxSessionJS::getName(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (WebxSession *session = WebxSession::Unwrap<WebxSession>(args.Holder())) {
    if (session->context) {
      std::string name = session->context->getName();
      args.GetReturnValue().Set(Nan::New(name).ToLocalChecked());
    }
  }
  else {
    Nan::ThrowError("Cannot get a session name on a closed session");
  }
}

void WebxSessionJS::dispatchEvent(const Nan::FunctionCallbackInfo<v8::Value> &args) {
  Nan::ThrowError("WebxSessionJS::dispatchEvent not implemented");
}

void WebxSessionJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (WebxSession *session = WebxSession::Unwrap<WebxSession>(args.Holder())) {
    session->context->close();
    session->context = 0;
  }
  else {
    Nan::ThrowError("Cannot close on a closed session");
  }
}

v8::Local<v8::Function> WebxSessionJS::CreatePrototype()
{

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(WebxSessionJS::New);
  tpl->SetClassName(Nan::New("WebxSession").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getName", WebxSessionJS::getName);
  Nan::SetPrototypeMethod(tpl, "dispatchEvent", WebxSessionJS::dispatchEvent);
  Nan::SetPrototypeMethod(tpl, "close", WebxSessionJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
