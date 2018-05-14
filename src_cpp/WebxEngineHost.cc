#include "./WebxEngineHost.h"

WebxEngineHost::WebxEngineHost(v8::Local<v8::Function> onNotification)
  : notifications(this, this->completeSync)
{
  using namespace v8;
  Isolate *isolate = Isolate::GetCurrent();

  this->connector = 0;
  this->onNotification.Reset(Isolate::GetCurrent(), onNotification);
}

WebxEngineHost::~WebxEngineHost()
{
  if (this->connector)
  {
    this->connector->disconnect();
    this->connector = 0;
  }
}

void WebxEngineHost::completeSync(uv_async_t *handle)
{
  using namespace v8;
  WebxEngineHost *_this = (WebxEngineHost *)handle->data;
  if (webx::IEvent *events = _this->notifications.flush())
  {
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<Function> onNotification = Local<Function>::New(isolate, _this->onNotification);
    for (webx::IEvent *ev = events; ev; ev = ev->next)
    {
      v8h::ObjectVisitor object(ev);
      Local<Value> argv[] = { Nan::New(ev->eventName()).ToLocalChecked(), object.data };
      onNotification->Call(isolate->GetCurrentContext()->Global(), 2, argv);
    }
    events->release();
  }
}

void WebxEngineHost::connect(const char *dllPath, const char *dllEntryPoint, const char *config)
{
  std::cout << "RUN: dll=" << dllPath << ", entry=" << dllEntryPoint << "\n";

  HMODULE hModule = LoadLibraryA(dllPath);
  if (hModule)
  {
    webx::tEngineConnectProc entryPoint = webx::tEngineConnectProc(GetProcAddress(hModule, dllEntryPoint));
    if (entryPoint)
    {
      this->connector = entryPoint(this, config);
    }
    else
    {
      Nan::ThrowError("Dll entryPoint is not found");
    }
  }
  else
  {
    Nan::ThrowError("Dll cannot be load (module or its dependencies may be invalid or unreachable)");
  }
}

void WebxEngineHost::notify(webx::IEvent *event)
{
  this->notifications.push(event);
}
