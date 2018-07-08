#include "./WebxSession.h"

WebxSession::WebxSession(v8::Local<v8::Function> handleEvent)
  : events(this, this->completeSync)
{
  using namespace v8;
  this->context = 0;
  this->engine = 0;
  this->handleEvent.Reset(Isolate::GetCurrent(), handleEvent);
}

WebxSession::~WebxSession()
{
  if (this->context)
  {
    this->context->close();
    this->context = 0;
  }
}

void WebxSession::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxSession *_this = (WebxSession *)handle->data;
  if (webx::IEvent *events = _this->events.flush())
  {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<Function> handleEvent = Local<Function>::New(isolate, _this->handleEvent);
    for (webx::IEvent *ev = events; ev; ev = ev->next)
    {
      v8h::ObjectVisitor object(ev);
      Local<Value> argv[] = { Nan::New(ev->eventName()).ToLocalChecked(), object.data };
      handleEvent->Call(isolate->GetCurrentContext()->Global(), 2, argv);
    }
    events->release();
  }
}

void WebxSession::notify(webx::IEvent *event)
{
  this->events.push(event);
}

void WebxSession::free() {
  printf("WebxSession leak !\n");
}
