#include "./WebxHttpTransaction.h"

Nan::Persistent<v8::Function> WebxHttpTransactionJS::constructor;

void WebxHttpTransactionJS::New(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (args.Length() != 5 || !args[0]->IsObject() || !args[1]->IsObject() || !args[2]->IsFunction() || !args[3]->IsFunction() || !args[4]->IsFunction())
    Nan::ThrowTypeError("Wrong arguments");
  if (!args.IsConstructCall())
    Nan::ThrowError("Is not a function");

  if (WebxSessionObjectWrap *session = WebxSessionObjectWrap::Unwrap<WebxSessionObjectWrap>(args[0]->ToObject())) {
    Local<Object> req = args[1]->ToObject();
    Local<Function> onSend = args[2].As<v8::Function>();
    Local<Function> onChunk = args[3].As<v8::Function>();
    Local<Function> onEnd = args[4].As<v8::Function>();

    WebxHttpTransaction *transaction = new WebxHttpTransaction(req, onSend, onChunk, onEnd);
    transaction->AttachObject(args.This());

    session->context->dispatchDatagram(transaction);

    args.GetReturnValue().Set(args.This());
  }
  else {
    Nan::ThrowError("Cannot create a transaction on a closed session");
  }
}

void WebxHttpTransactionJS::write(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (WebxHttpTransaction *transaction = WebxHttpTransaction::Unwrap<WebxHttpTransaction>(args.Holder())) {
    if (args.Length() != 1)
      Nan::ThrowTypeError("Wrong arguments");

    if (webx::IData *data = v8h::NewDataFromValue(args[0]))
    {
      if (transaction->requestHandler)
      {
        transaction->requestHandler->onData(transaction);
      }
      else printf(">>> Lost chunk\n");
      data->release();
      args.GetReturnValue().Set(true);
    }
    else
      Nan::ThrowError("Cannot write this type of chunk");
  }
  else {
    // Cannot write on a closed http transaction
    args.GetReturnValue().Set(false);
  }
}

void WebxHttpTransactionJS::close(const Nan::FunctionCallbackInfo<v8::Value> &args)
{
  using namespace v8;
  if (WebxHttpTransaction *transaction = WebxHttpTransaction::Unwrap<WebxHttpTransaction>(args.Holder())) {
    if (transaction->requestHandler) {
      transaction->requestStatus.data_end = 1;
      transaction->requestHandler->onComplete(transaction);
      transaction->requestHandler = 0;
    }
    args.GetReturnValue().Set(true);
  }
  else {
    // Cannot close on a closed http transaction
    args.GetReturnValue().Set(false);
  }
}

v8::Local<v8::Function> WebxHttpTransactionJS::CreatePrototype()
{
  using namespace v8;

  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(WebxHttpTransactionJS::New);
  tpl->SetClassName(Nan::New("WebxHttpTransaction").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "write", WebxHttpTransactionJS::write);
  Nan::SetPrototypeMethod(tpl, "close", WebxHttpTransactionJS::close);

  constructor.Reset(tpl->GetFunction());
  return tpl->GetFunction();
}
