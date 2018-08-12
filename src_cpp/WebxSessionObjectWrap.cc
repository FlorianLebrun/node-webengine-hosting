#include "./WebxSessionObjectWrap.h"

WebxSessionObjectWrap::WebxSessionObjectWrap(v8::Local<v8::Function> onEvent)
  : events(this, this->completeEvents_sync)
{
  using namespace v8;
  this->context = 0;
  this->onEvent.Reset(Isolate::GetCurrent(), onEvent);
}

WebxSessionObjectWrap::~WebxSessionObjectWrap() {
  memset(this, 0, sizeof(void*)); // Force VMT clean (for better crash)
  if (this->context)
  {
    this->context->close();
    this->context = 0;
  }
  this->onEvent.Reset();
}
