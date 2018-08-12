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

  if (webx::Ref<webx::IEvent> events = this->events.flush())
  {
    Local<Function> onEvent = Local<Function>::New(isolate, this->onEvent);
    for (webx::IEvent *ev = events; ev; ev = ev->next)
    {
      v8h::ObjectVisitor object(ev);
      Local<Value> argv[] = { Nan::New(ev->eventName()).ToLocalChecked(), object.data };
      onEvent->Call(isolate->GetCurrentContext()->Global(), 2, argv);
    }
  }
  if (this->events.is_completed()) {
    this->DettachObject();
    this->release();
  }
}

void WebxSession::dispatchTransaction(webx::IStream *request) {
  printf("!Error: Session host not support dispatchTransaction\n");
  request->close();
}

void WebxSession::dispatchWebSocket(webx::IStream *stream) {
  printf("!Error: Session host not support dispatchWebSocket\n");
  stream->close();
}

void WebxSession::notify(webx::IEvent *event)
{
  this->events.push(event);
}

bool WebxSession::disconnect()
{
  this->context = 0;
  this->events.complete();
  return true;
}

void WebxSession::free() {
  delete this;
}
