#ifndef vz_h_
#define vz_h_

#include <iostream>
#include <algorithm>
#include <nan.h>
#include <string>

#include "./engine-interfaces.h"
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
  if (value->IsArray())
  {
  }
  else if (value->IsString())
  {
    v8::Local<v8::String> s = value->ToString();
    v8::String::Utf8Value bytes(s);
    webx::IData *data = webx::IData::New(strlen(*bytes));
    memcpy(data->bytes, *bytes, data->size);
    return data;
  }
  return 0;
}

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
}

#endif