
#ifndef webx_engine_datagrams_h_
#define webx_engine_datagrams_h_

#include "./webx-releasables.h"

namespace webx
{
  class IData;
  class IDatagram;
  class IDatagramHandler;
  class IDatagramListener;
  
  struct tIOStatus {
    enum eFlags {
      DATA_END = 1,
      DATA_OVERFLOW = 2,
      ATTACHMENT_END = 1,
      ATTACHMENT_OVERFLOW = 2,
    };
    union {
      struct {
        unsigned data_end:1;
        unsigned data_overflow:1;
        unsigned attachment_end:1;
        unsigned attachment_overflow:1;
      };
      int flags;
    };
    tIOStatus(int flags = 0) {this->flags = flags;}
  };

  class IDatagram : public IReleasable
  {
  public:
    // API for receiving
    virtual bool accept(IDatagramHandler *handler) = 0;
    virtual void discard() = 0;

    // API for sending
    virtual bool send(IDatagram* response) {return false;}

    // API for incoming datas
    virtual IValue* getAttributs() = 0;
    virtual tIOStatus getStatus() = 0;
    virtual IData* pullData() = 0;
    virtual IDatagram* pullAttachment() {return 0;}
  };
  
  class IDatagramHandler
  {
  public:
    virtual void onData(IDatagram* from) {}
    virtual void onAttachment(IDatagram* from) {}
    virtual void onComplete(IDatagram* from) {}
    virtual void disconnect() = 0;
  };
  
  class IDatagramListener : public IReleasable
  {
  public:
    virtual IValue* getAttributs() = 0;
    virtual void onDatagram(IDatagram* datagram) = 0;
  };
  
  // Interface of data event
  class IData : public IReleasable
  {
  public:
    webx::Ref<IData> next;

    // getData: provide buffer, and return true when data is full, false when is chunked
    virtual bool getData(char* &buffer, uint32_t &size) = 0;

    // getDataCount: provide the number of chained data
    int getDataCount();

    static IData *New(const char* buffer, int size);
  };

  
  class DataQueue : public EventQueue<IData> {
  public:
    tIOStatus status;
  };

  inline int IData::getDataCount() {
    int count = 0;
    for(IData* data=this; data; data=(IData*)*this->next) count++;
    return count;
  }

  inline IData *IData::New(const char* buffer, int size)
  {
    class Data : public Releasable<IData>
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
}

#endif