#include "./WebxHttpTransaction.h"

Nan::Persistent<v8::Function> WebxHttpTransactionJS::constructor;

void WebxHttpTransaction::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 3 || !args[0]->IsObject() || !args[1]->IsObject() || !args[2]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  WebxEngineHost *host = Nan::ObjectWrap::Unwrap<WebxEngineHost>(args[0]->ToObject());
  Local<Object> req = args[1]->ToObject();
  Local<Function> onComplete = args[2].As<v8::Function>();

  WebxHttpTransaction *transaction = new WebxHttpTransaction(req, onComplete);
  transaction->Wrap(args.This());

  host->connector->dispatchTransaction(transaction);

  args.GetReturnValue().Set(args.This());
}

void WebxHttpTransactionJS::write(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxHttpTransaction *transaction = Nan::ObjectWrap::Unwrap<WebxHttpTransaction>(args.Holder());
  if (args.Length() != 1)
    Nan::ThrowTypeError("Wrong arguments");

  if (webx::IData *data = v8h::NewDataFromValue(args[0]))
  {
    if (transaction->opposite)
    {
      data->from = transaction;
      transaction->opposite->write(data);
    }
    else {
      printf(">>> Lost chunk\n"); 
    }
  }
  else
    Nan::ThrowError("Cannot write this type of chunk");
}

void WebxHttpTransactionJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  WebxHttpTransaction *transaction = Nan::ObjectWrap::Unwrap<WebxHttpTransaction>(args.Holder());
  if (transaction->opposite)
    transaction->opposite->close();
}

v8::Local<v8::Function> WebxHttpTransactionJS::CreatePrototype()
{
  using namespace v8;

  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(WebxHttpTransaction::New);
  tpl->SetClassName(Nan::New("WebxHttpTransaction").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "write", WebxHttpTransactionJS::write);
  Nan::SetPrototypeMethod(tpl, "close", WebxHttpTransactionJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
