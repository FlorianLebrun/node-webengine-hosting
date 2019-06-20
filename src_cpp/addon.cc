
#include "./v8helper.h"
#include <windows.h>

#include "./WebxEngine.h"
#include "./WebxHttpTransaction.h"

void Init(v8::Local<v8::Object> exports)
{
	Nan::HandleScope scope;
  exports->Set(Nan::New("WebxEngine").ToLocalChecked(), WebxEngineJS::CreatePrototype());
  exports->Set(Nan::New("WebxSession").ToLocalChecked(), WebxSessionJS::CreatePrototype());
	exports->Set(Nan::New("WebxHttpTransaction").ToLocalChecked(), WebxHttpTransactionJS::CreatePrototype());
}

NODE_MODULE(addon, Init)
