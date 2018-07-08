
#ifndef webx_engine_interfaces_h_
#define webx_engine_interfaces_h_

#include <functional>
#include <iostream>

#include "./webx-attributs.h"
#include "./webx-releasables.h"
#include "./webx-events.h"

namespace webx
{
class IReleasable;
class IAttributs;

class IPipeable;
class IStream;
class IHttpTransaction;

class ISessionContext;
class IEngineContext;
class ISessionHost;
class IEngineHost;

class IPipeable : public IAttributs
{
public:
  // Connect the pipeline to an opposite stream
  virtual bool connect(IStream *stream) = 0;
};

class IStream : public IPipeable
{
public:
  // Push a data packet on the stream
  virtual bool write(IData *data) = 0;

  // Notify the end of stream (no write can be done after)
  virtual void close() = 0;
};

class IHttpTransaction : public IPipeable
{
public:
  // Provide the stream for response delivery
  virtual IStream *getResponse() = 0;
};

class IEndPoint : public IAttributs
{
public:
  virtual void dispatchTransaction(IHttpTransaction *transaction) = 0;
  virtual void dispatchWebSocket(IStream *stream) = 0;
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

class IEngineContext
{
public:
  virtual const char *getName() = 0;
  virtual ISessionContext *createMainSession(ISessionHost *host, const char *config) = 0;
  virtual ISessionContext *createSession(ISessionHost *host, const char* name, const char *config) = 0;
  virtual bool close() = 0;
};

class IEngineHost
{
public:
  virtual void notify(IEvent *event) = 0;
  virtual bool disconnect() = 0;
};

typedef IEngineContext *(*tEngineConnectProc)(IEngineHost *host, const char *config);

} // namespace webx

#endif