
#ifndef webx_engine_objects_h_
#define webx_engine_objects_h_

#include <map>
#include <stdint.h>
#include <string>
#include <atomic>

namespace webx
{

  class IReleasable
  {
  public:

    // Retain the object
    virtual void retain() = 0;

    // Release the object, true when object is deleted
    virtual void release() = 0;
  };

  template <class CReleasable>
  class Ref {
    template <class CReleasable>
    friend Ref<CReleasable> New(CReleasable* object);
    CReleasable* _object;
  public:
    Ref() {
      this->_object = 0;
    }
    Ref(Ref<CReleasable>& ref) {
      this->_object = ref._object;
      ref._object = 0;
    }
    ~Ref() {
      if (this->_object) this->_object->release();
    }
    void New(CReleasable* object) {
      this->_object = object;
    }
    CReleasable* operator = (CReleasable* object) {
      if (this->_object) this->_object->release();
      if (object) object->retain();
      return this->_object = object;
    }
    CReleasable* operator -> () {
      return this->_object;
    }
    operator bool() {
      return this->_object != 0;
    }
    template <class Ty>
    Ty* cast() {
      return (Ty*)this->_object;
    }
    operator CReleasable*() {
      return this->_object;
    }
  };

  template <class CReleasable>
  inline Ref<CReleasable> New(CReleasable* object) {
    Ref<CReleasable> ref;
    ref._object = object;
    return ref;
  }

#ifdef _DEBUG
  static std::atomic<int> _s_object_count;
#endif

  template <class CReleasable>
  class Releasable : public CReleasable
  {
  public:
    std::atomic<intptr_t> nref;

    Releasable() : nref(1)
    {
#ifdef _DEBUG
      _s_object_count++;
      printf("(+) object %d\n", (int)_s_object_count);
      if (_s_object_count > 20) {
        printf("leak! %d\n", (int)_s_object_count);
      }
#endif
    }
    virtual void retain() override
    {
      this->nref++;
    }
    virtual void release() override
    {
      if ((--this->nref) <= 0) {
#ifdef _DEBUG
        _s_object_count--;
        printf("(-) object %d\n", (int)_s_object_count);
        if (this->nref < 0) {
          printf("(!) crash risk\n");
        }
#endif
        this->free();
      }
    }
    virtual void free() = 0;
  };
} // namespace webx

#endif
