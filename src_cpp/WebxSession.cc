#include "./WebxSession.h"

TRACE_LEAK(static std::atomic<intptr_t> leakcount = 0);

WebxSession::WebxSession(v8::Local<v8::Function> onEvent)
  : WebxSessionObjectWrap(onEvent)
{
  this->engine = 0;
  TRACE_LEAK(printf("<WebxSession %d>\n", int(++leakcount)));
}

WebxSession::~WebxSession()
{
  TRACE_LEAK(printf("<WebxSession %d>\n", int(--leakcount)));
}

void WebxSession::completeEvents()
{
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (webx::Ref<webx::IEvent> events = webx::New(this->events.flush()))
  {
    Local<Function> onEvent = Local<Function>::New(isolate, this->onEvent);
    for (webx::IEvent *ev = events; ev; ev = ev->next)
    {
      v8::Local<v8::Object> data = v8h::createObjectFromValue(ev);
      Local<Value> argv[] = { Nan::New(ev->eventName()).ToLocalChecked(), data };
      onEvent->Call(isolate->GetCurrentContext()->Global(), 2, argv);
    }
  }
  if (this->events.is_completed()) {
    this->DettachObject();
    this->release();
  }
}

void WebxSession::dispatchDatagram(webx::IDatagram *datagram) {
  printf("!Error: Session host not support dispatchTransaction\n");
  datagram->discard();
}

void WebxSession::dispatchEvent(webx::IEvent *event)
{
  this->events.push(event);
}

bool WebxSession::disconnect(webx::ISession* session)
{
  if (session == this->context) {
    this->context = 0;
    this->events.complete();
    return true;
  }
  return false;
}

void WebxSession::free() {
  delete this;
}
