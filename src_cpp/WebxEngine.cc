#include "./WebxEngine.h"

TRACE_LEAK(static std::atomic<intptr_t> leakcount = 0);

WebxEngine::WebxEngine(v8::Local<v8::Function> onEvent)
  : WebxSessionObjectWrap(onEvent)
{
  this->instance = 0;
  TRACE_LEAK(printf("<WebxEngine %d>\n", int(++leakcount)));
}

WebxEngine::~WebxEngine()
{
  TRACE_LEAK(printf("<WebxEngine %d>\n", int(--leakcount)));
}

void WebxEngine::completeEvents()
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

void WebxEngine::connect(const char *dllPath, const char *dllEntryPoint, const char *config)
{
  std::cout << "RUN: dll=" << dllPath << ", entry=" << dllEntryPoint << "\n";

  HMODULE hModule = LoadLibrary(dllPath);
  if (hModule)
  {
    webx::tEngineConnectProc entryPoint = webx::tEngineConnectProc(GetProcAddress(hModule, dllEntryPoint));
    if (entryPoint)
    {
      this->instance = entryPoint(this, config);
      this->context = this->instance;
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

void WebxEngine::dispatchDatagram(webx::IDatagram *datagram) {
  printf("!Error: Engine host not support dispatchDatagram\n");
  datagram->discard();
}

void WebxEngine::notify(webx::IEvent *event)
{
  this->events.push(event);
}
void WebxEngine::free() {
  delete this;
}


bool WebxEngine::disconnect()
{
  this->context = 0;
  this->events.complete();
  return true;
}
