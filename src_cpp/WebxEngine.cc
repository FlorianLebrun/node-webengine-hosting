#include "./WebxEngine.h"
#include "./WebxSession.h"

WebxEngine::WebxEngine(v8::Local<v8::Function> handleEvent)
  : events(this, this->completeSync)
{
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();

  this->context = 0;
  this->mainSession = 0;
  this->handleEvent.Reset(Isolate::GetCurrent(), handleEvent);
}

WebxEngine::~WebxEngine()
{
  if (this->context)
  {
    this->context->close();
    this->context = 0;
  }
}

void WebxEngine::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxEngine *_this = (WebxEngine *)handle->data;
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

void WebxEngine::connect(const char *dllPath, const char *dllEntryPoint, const char *config)
{
  std::cout << "RUN: dll=" << dllPath << ", entry=" << dllEntryPoint << "\n";

  HMODULE hModule = LoadLibrary(dllPath);
  if (hModule)
  {
    webx::tEngineConnectProc entryPoint = webx::tEngineConnectProc(GetProcAddress(hModule, dllEntryPoint));
    if (entryPoint)
    {
      this->context = entryPoint(this, config);
    }
    else
    {
      char msg[512];
      sprintf_s(msg, sizeof(msg), "Error (%d) at GetProcAddress: Dll entryPoint is not found)", GetLastError());
      Nan::ThrowError("Dll entryPoint is not found");
    }
  }
  else
  {
    char msg[512];
    sprintf_s(msg, sizeof(msg), "Error (%d) at LoadLibraryA: Dll cannot be load (module or its dependencies may be invalid or unreachable)", GetLastError());
    Nan::ThrowError(msg);
  }
}

void WebxEngine::notify(webx::IEvent *event)
{
  this->events.push(event);
}
