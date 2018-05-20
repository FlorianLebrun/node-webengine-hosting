
#include "./engine-attributs.h"
#include "./engine-events.h"

#ifndef webx_engine_interfaces_h_
#define webx_engine_interfaces_h_

#include <functional>
#include <iostream>

namespace webx
{
  class IPipeable;
  class IStream;
  class IHttpTransaction;

  class IPipeable : public IAttributs
  {
  public:
    virtual void setOpposite(IStream *stream) = 0;
  };

  class IStream : public IPipeable
  {
  public:
    virtual bool write(IData *data) = 0;
    virtual void close() = 0;
  };

  class IHttpTransaction : public IPipeable
  {
  public:
    virtual IStream *getResponse() = 0;
  };

  class IServiceProvider
  {
  public:
    virtual void dispatchTransaction(IHttpTransaction *transaction) = 0;
    virtual void dispatchWebSocket(IStream *stream) = 0;
  };

  class IEngineContext : public IServiceProvider
  {
  public:
  };

  class IEngineConnector : public IServiceProvider
  {
  public:
    virtual const char *getName() = 0;
    virtual IEngineContext *createContext() = 0;
    virtual bool disconnect() = 0;
  };

  class IEngineHost : public IServiceProvider
  {
  public:
    virtual void notify(IEvent* event) = 0;
  };

  typedef IEngineConnector *(*tEngineConnectProc)(IEngineHost *host, const char *args);

} // namespace webx

#endif