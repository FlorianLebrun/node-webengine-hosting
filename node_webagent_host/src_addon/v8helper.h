#ifndef vz_h_
#define vz_h_

#include <iostream>
#include <algorithm>
#include <nan.h>
#include <string>

#include <webagent-hosting/webx.h>
#include "./spinlock.h"

#pragma error (disable : 145)

#define TRACE_LEAK(x) //x

namespace v8h
{
  class ObjectWrap {
  public:
    v8::Persistent<v8::Object> handle;

    ~ObjectWrap() {
      this->handle.Reset();
    }
    inline void AttachObject(v8::Local<v8::Object> object) {
      assert(this->handle.IsEmpty());
      assert(object->InternalFieldCount() > 0);
      Nan::SetInternalFieldPointer(object, 0, this);
      this->handle.Reset(v8::Isolate::GetCurrent(), object);
    }
    inline void DettachObject() {
      assert(!this->handle.IsEmpty());
      Nan::SetInternalFieldPointer(this->toObject(), 0, 0);
      this->handle.Reset();
    }
    inline v8::Local<v8::Object> toObject() const {
      return Nan::New(this->handle);
    }
    template <class T>
    static inline T* Unwrap(v8::Local<v8::Object> object) {
      assert(!object.IsEmpty());
      assert(object->InternalFieldCount() > 0);
      void* ptr = Nan::GetInternalFieldPointer(object, 0);
      ObjectWrap* wrap = static_cast<ObjectWrap*>(ptr);
      return static_cast<T*>(wrap);
    }
  };

  inline v8::Local<v8::String> MakeString(const char *value, int length = -1)
  {
    return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), value, v8::String::kNormalString, length);
  }
  inline std::string GetUtf8(v8::Local<v8::Value> value)
  {
    v8::Local<v8::String> s = value->ToString(v8::Isolate::GetCurrent());
    return *v8::String::Utf8Value(v8::Isolate::GetCurrent(), s);
  }
  inline v8::Local<v8::Value> GetIn(v8::Local<v8::Object> object, const char *key)
  {
    return object->Get(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), key));
  }
  inline bool SetIn(v8::Local<v8::Object> object, const char *key, v8::Local<v8::Value> value)
  {
    return object->Set(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), key), value);
  }
  inline v8::Local<v8::Value> GetIn(v8::Local<v8::Object> object, v8::Local<v8::String> key)
  {
    return object->Get(key);
  }

  inline webx::IData *NewDataFromValue(v8::Local<v8::Value> value)
  {
    if (value->IsObject())
    {
      char* bufferData = node::Buffer::Data(value);
      size_t bufferLength = node::Buffer::Length(value);
      return webx::IData::New(bufferData, bufferLength);
    }
    else if (value->IsString())
    {
      v8::Local<v8::String> s = value->ToString(v8::Isolate::GetCurrent());
      v8::String::Utf8Value bytes(v8::Isolate::GetCurrent(), s);
      return webx::IData::New(*bytes, bytes.length());
    }
    return 0;
  }

  template <class CEvent = webx::IEvent>
  class EventQueue : public webx::EventQueue<CEvent>
  {
    SpinLock lock;
    uv_async_t async;
    bool completed;

  public:
    EventQueue(void* completeContext, uv_async_cb completeSync)
    {
      uv_loop_t *loop = uv_default_loop();
      this->async.data = completeContext;
      this->completed = false;
      uv_async_init(loop, &this->async, completeSync);
    }
    bool is_completed() {
      return this->completed;
    }
    void pushNew(CEvent *data) {
      this->lock.lock();
      this->webx::EventQueue<CEvent>::pushNew(data);
      this->lock.unlock();
      uv_async_send(&this->async);
    }
    void pushRetain(CEvent *data) {
      this->lock.lock();
      this->webx::EventQueue<CEvent>::pushRetain(data);
      this->lock.unlock();
      uv_async_send(&this->async);
    }
    webx::Ref<CEvent> pop() {
      this->lock.lock();
      webx::Ref<CEvent> data = this->webx::EventQueue<CEvent>::pop();
      this->lock.unlock();
      return data;
    }
    CEvent* flush() {
      this->lock.lock();
      CEvent* data = this->webx::EventQueue<CEvent>::flush();
      this->lock.unlock();
      return data;
    }
    void complete() {
      this->completed = true;
      uv_async_send(&this->async);
    }
    void close(uv_close_cb close_cb) {
      uv_close((uv_handle_t *)&this->async, close_cb);
    }
  };

  inline v8::Local<v8::Object> createObjectFromValue(webx::IValue* value) {
    v8::Local<v8::Object> data(Nan::New<v8::Object>());
    value->foreach([&data](const webx::IValue& key, const webx::IValue& value) {
      std::string vkey = key.toString();
      std::string vvalue = value.toString();
      data->Set(Nan::New(vkey).ToLocalChecked(), Nan::New(vvalue).ToLocalChecked());
    });
    return data;
  }

} // namespace v8h

#endif