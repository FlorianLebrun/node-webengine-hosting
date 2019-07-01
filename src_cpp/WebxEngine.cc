#include "./WebxEngine.h"

WebxEngine::WebxEngine(v8::Local<v8::Function> onEvent)
  : WebxSessionObjectWrap(onEvent)
{
}

WebxEngine::~WebxEngine()
{
}

void WebxEngine::completeEvents()
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
      sprintf_s(msg, sizeof(msg), 
        "Error (%d) at GetProcAddress: Dll entryPoint '%s' is not found)",
        GetLastError(),
        dllEntryPoint
      );
      Nan::ThrowError("Dll entryPoint is not found");
    }
  }
  else
  {
    char msg[512];
    sprintf_s(msg, sizeof(msg),
      "Error (%d) at LoadLibraryA: Dll cannot be load (module or its dependencies may be invalid or unreachable) at '%s'", 
      GetLastError(),
      dllPath
    );
    Nan::ThrowError(msg);
  }
}

void WebxEngine::dispatchDatagram(webx::IDatagram *datagram) {
  printf("!Error: Engine host not support dispatchDatagram\n");
  datagram->discard();
}

void WebxEngine::dispatchEvent(webx::IEvent *event)
{
  this->events.pushRetain(event);
}

bool WebxEngine::disconnect(webx::ISession* session)
{
  this->context = 0;
  this->events.complete();
  return true;
}

bool WebxEngine::terminate() 
{
  return false;
}