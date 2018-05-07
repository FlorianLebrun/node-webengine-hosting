
#ifndef engine_interfaces_h_
#define engine_interfaces_h_

#include <functional>
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>

namespace webx
{
class IAttributs;
class IPipeable;
class IStream;
class IHttpTransaction;

class IAttributs
{
public:
  class IVisitor
  {
  public:
    virtual void visitInt(const char *name, int64_t value);
    virtual void visitFloat(const char *name, double value);
    virtual void visitString(const char *name, const char *value) = 0;
  };

  virtual int32_t getAttributCount() = 0;
  virtual bool visitAttributs(IVisitor *visitor) = 0;
  virtual bool hasAttribut(const char *name) = 0;
  virtual bool removeAttribut(const char *name) = 0;

  virtual int64_t getAttributInt(const char *name) = 0;
  virtual bool setAttributInt(const char *name, int64_t value) = 0;

  virtual double getAttributFloat(const char *name) = 0;
  virtual bool setAttributFloat(const char *name, double value) = 0;

  virtual const char *getAttributString(const char *name) = 0;
  virtual bool setAttributString(const char *name, const char *value, int size = -1) = 0;
};

class IPipeable : public IAttributs
{
public:
  virtual void setOpposite(IStream *stream) = 0;
};

class IData : public IAttributs
{
public:
  IData *next;
  IPipeable *from;
  uint32_t allocated;
  uint32_t size;
  char *bytes;

  virtual void release() = 0;

  static IData *New(int size);
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
  virtual void complete(int statusCode) = 0;
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
  virtual void notify(const char *event, const void *bytes = 0, int32_t length = -1) = 0;
};

typedef IEngineConnector *(*tEngineConnectProc)(IEngineHost *host, const char *args);

#include "./engine-interfaces-impl.h"

} // namespace webx

#endif