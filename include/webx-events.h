
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
    virtual int eventID() override { return dataEventID; }
    virtual const char *eventName() override { return "data"; }

    // getData: provide buffer, and return true when data is full, false when is chunked
    virtual bool getData(char* &buffer, uint32_t &size) = 0;

    // getOrigin: provide the pipe which send the data
    virtual IPipeable* getOrigin() { return 0; }

    static IData *New(const char* buffer, int size);
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

  inline IData *IData::New(const char* buffer, int size)
  {
    class Data : public NoAttributs<Releasable<IData>>
    {
    public:
      uint32_t size;
      char buffer[1];
      Data(const char* buffer, uint32_t size)
      {
        this->next = 0;
        this->size = size;
        memcpy(this->buffer, buffer, size);
      }
      virtual bool getData(char* &buffer, uint32_t &size) {
        buffer = this->buffer;
        size = this->size;
        return true;
      }
      virtual void free() override
      {
        ::free(this);
      }
    };
    return new (::malloc(sizeof(Data) + size)) Data(buffer, size);
  }
} // namespace webx
#endif