#include "./WebxSessionObjectWrap.h"

WebxSessionObjectWrap::WebxSessionObjectWrap(v8::Local<v8::Function> onEvent)
  : events(this, this->completeEvents_sync)
{
  using namespace v8;
  this->context = 0;
  this->onEvent.Reset(Isolate::GetCurrent(), onEvent);
}
