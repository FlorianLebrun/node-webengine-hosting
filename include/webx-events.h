
#ifndef webx_engine_events_h_
#define webx_engine_events_h_

#include "./webx-releasables.h"

namespace webx
{
  // Event queue
  template <class CEvent>
  class EventQueue;

  // Interface of event
  class IEvent : public IReleasable, public IValue
  {
  public:
    webx::Ref<IEvent> next;
    virtual tTypeId getTypeId() const {
      return tTypeId::mapId;
    }
    virtual const char *eventName() { 
      return "message"; 
    }
  };

  // -------------------------------------------------
  // Implementations
  // -------------------------------------------------
  template <class CEvent>
  class BuiltinEvent : public Releasable<CEvent> {

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
    int count()
    {
      int c = 0;
      for(IEvent* evt=this->first;evt;evt=evt->next) c++;
      return c;
    }
    operator bool() {
      return this->first != 0;
    }
  };

} // namespace webx
#endif