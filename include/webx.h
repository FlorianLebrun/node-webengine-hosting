
#ifndef webx_engine_interfaces_h_
#define webx_engine_interfaces_h_
#pragma pack(push)
#pragma pack()

#include <functional>
#include <iostream>

#include "./webx-value.h"
#include "./webx-releasables.h"
#include "./webx-events.h"
#include "./webx-datagrams.h"

namespace webx
{
  class ISession;
  class IEngine;
  class ISessionHost;
  class IEngineHost;

  class IEndPoint : public IReleasable
  {
  public:
    virtual IValue* getAttributs() {return &IValue::Undefined;}
    virtual void dispatchDatagram(IDatagram *transaction) = 0;
    //virtual void dispatchListener(IDatagramListener *listener) = 0;
    virtual void dispatchEvent(IEvent *event) = 0;
  };

  class ISession : public IEndPoint
  {
  public:
    virtual std::string getName() = 0;
    virtual bool hasSessionAffinity() = 0;
    virtual bool close() = 0;
  };

  class ISessionHost : public IEndPoint
  {
  public:
    virtual bool disconnect(ISession* session) = 0;
  };

  class ISessionType : public IReleasable
  {
  public:
    virtual std::string getName() = 0;
    virtual IValue* getAttributs() {return &IValue::Undefined;}
  };

  class IEngine : public ISession
  {
  public:
    virtual bool hasSessionAffinity(std::string type) = 0;
    virtual ISessionType* createSessionType(std::string type, const char* json) = 0;
    virtual void visitSessionTypes(const std::function<void(ISessionType*)>& visitor) = 0;
    
    virtual ISession *createSession(std::string type, std::string name, ISessionHost *host) = 0;
    virtual void visitSessions(const std::function<void(ISession*)>& visitor) = 0;
  };

  class IEngineHost : public ISessionHost
  {
  public:
    virtual bool terminate() = 0;
  };

  typedef IEngine *(*tEngineConnectProc)(IEngineHost *host, const char *config);

} // namespace webx

#pragma pack(pop)
#endif