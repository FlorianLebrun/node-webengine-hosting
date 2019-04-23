
#ifndef webx_engine_interfaces_h_
#define webx_engine_interfaces_h_

#include <functional>
#include <iostream>

#include "./webx-attributs.h"
#include "./webx-releasables.h"
#include "./webx-events.h"
#include "./webx-datagrams.h"

namespace webx
{
  class ISessionContext;
  class IEngineContext;
  class ISessionHost;
  class IEngineHost;

  class IEndPoint : public IAttributs
  {
  public:
    virtual void dispatchDatagram(IDatagram *transaction) = 0;
    //virtual void dispatchListener(IDatagramListener *listener) = 0;
    virtual void notify(IEvent *event) = 0;
  };

  class ISessionContext : public IEndPoint
  {
  public:
    virtual const char *getName() = 0;
    virtual bool close() = 0;
  };

  class ISessionHost : public IEndPoint
  {
  public:
    virtual bool disconnect() = 0;
  };

  class IEngineContext : public ISessionContext
  {
  public:
    virtual ISessionContext *createSession(ISessionHost *host, const char* name, const char *config) = 0;
  };

  class IEngineHost : public ISessionHost
  {
  public:

  };

  typedef IEngineContext *(*tEngineConnectProc)(IEngineHost *host, const char *config);

} // namespace webx

#endif