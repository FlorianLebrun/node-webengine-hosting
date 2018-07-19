
#ifndef webx_engine_events_h_
#define webx_engine_events_h_

#include "./webx-releasables.h"

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
    webx::Ref<IEvent> next;
    virtual int eventID() { return undefinedEventID; }
    virtual const char *eventName() { return "undefined"; }
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
  class BuiltinEvent : public BuiltinAttributs<Releasable<CEvent>> {

  };

  template <class CEvent>
  class EventQueue
  {
  public:
    CEvent* first, *_;
    EventQueue() {
      this->first = 0;
    }
    ~EventQueue() {
      if (this->first) this->first->release();
    }
    void push(CEvent *data)
    {
      data->retain();
      data->next = 0;
      if (this->first)
        this->_->next = data;
      else
        this->first = data;
      this->_ = data;
    }
    Ref<CEvent> pop()
    {
      CEvent* data = this->first;
      if (data)
      {
        this->first = data->next;
        data->next = 0;
      }
      return New(data);
    }
    Ref<CEvent> flush()
    {
      CEvent* data = this->first;
      this->first = 0;
      return New(data);
    }
    operator bool() {
      return this->first != 0;
    }
  };

  inline IData *IData::New(int size)
  {
    class Data : public NoAttributs<Releasable<IData>>
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
      virtual void free() override
      {
        ::free(this);
      }
    };
    return new (::malloc(sizeof(Data) + size)) Data(size);
  }
} // namespace webx
#endif