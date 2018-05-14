
#ifndef webx_engine_events_h_
#define webx_engine_events_h_

#include "./engine-attributs.h"

namespace webx
{
  class IPipeable;
}

namespace webx
{
  // Events ID
  static const int undefinedEventID = 0;
  static const int dataEventID = 1;

  // Event queue
  template <class CEvent>
  class EventQueue;

  // Interface of event
  class IEvent : public IAttributs
  {
  public:
    IEvent *next;
    virtual int eventID() { return undefinedEventID; }
    virtual const char *eventName() { return "undefined"; }
    virtual void release() = 0;
  };

  // Interface of data event
  class IData : public IEvent
  {
  public:
    IPipeable *from;
    uint32_t allocated;
    uint32_t size;
    char *bytes;

    virtual int eventID() override { return dataEventID; }
    virtual const char *eventName() override { return "data"; }
    static IData *New(int size);
  };

  // -------------------------------------------------
  // Implementations
  // -------------------------------------------------
  template <class CEvent>
  class BuiltinEvent : public BuiltinAttributs<CEvent> {
    virtual void release() override
    {
      delete this;
    }
  };

  template <class CEvent>
  class EventQueue
  {
  public:
    CEvent *first, *_;
    EventQueue()
    {
      this->first = 0;
    }
    ~EventQueue()
    {
      IEvent *d = this->first;
      while (d)
      {
        IEvent *dn = d->next;
        d->release();
        d = dn;
      }
    }
    void push(CEvent *data)
    {
      data->next = 0;
      if (this->first)
        this->_->next = data;
      else
        this->first = data;
      this->_ = data;
    }
    CEvent *pop()
    {
      CEvent *data = this->first;
      if (data)
      {
        this->first = (CEvent*)data->next;
        data->next = 0;
      }
      return data;
    }
    CEvent *flush()
    {
      CEvent *data = this->first;
      this->first = 0;
      return data;
    }
    operator bool() {
      return this->first != 0;
    }
  };

  inline IData *IData::New(int size)
  {
    class Data : public NoAttributs<IData>
    {
    public:
      char buffer[1];
      Data(int size)
      {
        this->next = 0;
        this->from = 0;
        this->size = size;
        this->bytes = this->buffer;
      }
      virtual void release() override
      {
        if (this->next)
          this->next->release();
        ::free(this);
      }
    };
    return new (::malloc(sizeof(IData) + size)) Data(size);
  }
} // namespace webx
#endif