#ifndef vz_h_
#define vz_h_

#include <iostream>
#include <algorithm>
#include <nan.h>
#include <string>

#include "../include/webx.h"
#include "./spinlock.h"

namespace v8h
{
  inline v8::Local<v8::String> MakeString(const char *value, int length = -1)
  {
    return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), value, v8::String::kNormalString, length);
  }
  inline std::string GetUtf8(v8::Local<v8::Value> value)
  {
    v8::Local<v8::String> s = value->ToString();
    return *v8::String::Utf8Value(s);
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
      webx::IData *data = webx::IData::New(bufferLength);
      memcpy(data->bytes, bufferData, data->size);
      return data;
    }
    else if (value->IsString())
    {
      v8::Local<v8::String> s = value->ToString();
      v8::String::Utf8Value bytes(s);
      webx::IData *data = webx::IData::New(bytes.length());
      memcpy(data->bytes, *bytes, data->size);
      return data;
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
    void push_idle(CEvent *data) {
      this->lock.lock();
      this->webx::EventQueue<CEvent>::push(data);
      this->lock.unlock();
    }
    void push(CEvent *data) {
      this->lock.lock();
      this->webx::EventQueue<CEvent>::push(data);
      this->lock.unlock();
      uv_async_send(&this->async);
    }
    webx::Ref<CEvent> pop() {
      this->lock.lock();
      webx::Ref<CEvent> data = this->webx::EventQueue<CEvent>::pop();
      this->lock.unlock();
      return data;
    }
    webx::Ref<CEvent> flush() {
      this->lock.lock();
      webx::Ref<CEvent> data = this->webx::EventQueue<CEvent>::flush();
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

  class ObjectVisitor : public webx::IAttributsVisitor
  {
  public:
    v8::Local<v8::Object> data;

    ObjectVisitor(webx::IAttributs *notification)
      : data(Nan::New<v8::Object>())
    {
      notification->visitAttributs(this);
    }
    virtual void visitInt(const char *name, int64_t value) override
    {
      this->data->Set(Nan::New(name).ToLocalChecked(), Nan::New((int32_t)value));
    }
    virtual void visitFloat(const char *name, double value) override
    {
      this->data->Set(Nan::New(name).ToLocalChecked(), Nan::New(value));
    }
    virtual void visitObject(const char *name, webx::IAttributs *value) override
    {
      v8h::ObjectVisitor object(value);
      this->data->Set(Nan::New(name).ToLocalChecked(), object.data);
    }
    virtual void visitString(const char *name, const char *value) override
    {
      this->data->Set(Nan::New(name).ToLocalChecked(), Nan::New(value).ToLocalChecked());
    }
  };

  template <class T>
  class StringMapBasedAttributs : public webx::StringMapBasedAttributs<T>
  {
  public:
    void setAttributStringV8(v8::Local<v8::Value> name, v8::Local<v8::Value> value)
    {
      this->attributs[GetUtf8(name)] = GetUtf8(value);
    }
    void setAttributStringV8(const char *name, v8::Local<v8::Value> value)
    {
      this->attributs[name] = GetUtf8(value);
    }
  };
} // namespace v8h

#endif