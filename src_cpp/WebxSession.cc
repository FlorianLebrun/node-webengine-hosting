#include "./WebxSession.h"

WebxSession::WebxSession(v8::Local<v8::Function> onEvent)
  : events(this, this->completeEvents_sync)
{
  using namespace v8;
  this->context = 0;
  this->engine = 0;
  this->onEvent.Reset(Isolate::GetCurrent(), onEvent);
}

WebxSession::~WebxSession()
{
  if (this->context)
  {
    this->context->close();
    this->context = 0;
  }
}

void WebxSession::completeEvents()
{
  using namespace v8;
  if (webx::Ref<webx::IEvent> events = this->events.flush())
  {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<Function> onEvent = Local<Function>::New(isolate, this->onEvent);
    for (webx::IEvent *ev = events; ev; ev = ev->next)
    {
      v8h::ObjectVisitor object(ev);
      Local<Value> argv[] = { Nan::New(ev->eventName()).ToLocalChecked(), object.data };
      onEvent->Call(isolate->GetCurrentContext()->Global(), 2, argv);
    }
  }
}

void WebxSession::notify(webx::IEvent *event)
{
  this->events.push(event);
}

void WebxSession::free() {
  printf("WebxSession leak !\n");
}
