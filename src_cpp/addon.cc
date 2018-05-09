
#include "./v8helper.h"
#include <windows.h>

#include "./WebxEngineHost.h"
#include "./WebxHttpTransaction.h"
#include "./WebxWebSocketStream.h"

void Init(v8::Local<v8::Object> exports)
{
	Nan::HandleScope scope;

	exports->Set(Nan::New("WebxEngineHost").ToLocalChecked(), WebxEngineHostJS::CreatePrototype());
	exports->Set(Nan::New("WebxHttpTransaction").ToLocalChecked(), WebxHttpTransactionJS::CreatePrototype());
	exports->Set(Nan::New("WebxWebSocketStream").ToLocalChecked(), WebxWebSocketStreamJS::CreatePrototype());
}

NODE_MODULE(addon, Init)
