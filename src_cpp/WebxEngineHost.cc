#include "./WebxEngineHost.h"

WebxEngineHost::WebxEngineHost()
{
  this->connector = 0;

  uv_loop_t *loop = uv_default_loop();
/*  this->queue_async.data = this;
  uv_async_init(loop, &this->queue_async, completeSync);
  uv_mutex_init(&this->queue_mutex);*/
}

WebxEngineHost::~WebxEngineHost()
{
  if (this->connector) {
    this->connector->disconnect();
    this->connector = 0;
  }
}

void WebxEngineHost::completeSync(uv_async_t *handle){

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

void WebxEngineHost::notify(const char *event, const void *bytes, int32_t length)
{
  if (bytes)
    std::cout << "notify '" << event << "' : '" << bytes << "'\n";
  else
    std::cout << "notify '" << event << "'\n";
}
