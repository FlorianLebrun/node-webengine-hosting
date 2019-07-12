
#ifndef webx_engine_objects_h_
#define webx_engine_objects_h_

#include <map>
#include <stdint.h>
#include <string>
#include <atomic>

#ifdef _DEBUG
#define _RELEASABLE_DEBUG
#define _RELEASABLE_DEBUG_ONLY(x) x
#define _RELEASABLE_DEBUG_LEAK_OVERFLOW 8
#else
#define _RELEASABLE_DEBUG_ONLY(x)
#endif

namespace webx
{

  class IReleasable
  {
  public:

    // Retain the object
    virtual void retain() = 0;

    // Release the object, true when object is deleted
    virtual void release() = 0;

    // Release the object, true when object is deleted
    virtual void __check__alive() {}
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
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      if (this->_object) this->_object->release();
    }
    void Box(CReleasable* object) {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      this->_object = object;
    }
    CReleasable* flush() {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      CReleasable* object = this->_object;
      this->_object = 0;
      return object;
    }
    CReleasable* operator = (CReleasable* object) {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      if (this->_object) this->_object->release();
      if (object) object->retain();
      return this->_object = object;
    }
    CReleasable* operator -> () {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      return this->_object;
    }
    CReleasable* operator * () {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      return this->_object;
    }
    operator bool() {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      return this->_object != 0;
    }
    template <class Ty>
    Ty* cast() {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      return (Ty*)this->_object;
    }
    operator CReleasable*() {
      _RELEASABLE_DEBUG_ONLY(this->__check__alive());
      return this->_object;
    }
    void __check__alive() {
      if (this->_object) this->_object->__check__alive();
    }
  };

  template <class CReleasable>
  inline Ref<CReleasable> New(CReleasable* object) {
    Ref<CReleasable> ref;
    ref._object = object;
    return ref;
  }

#ifdef _RELEASABLE_DEBUG
  int _report_releasable_count(const type_info& infos);
  int _report_releasable_new(const type_info& infos);
  int _report_releasable_delete(const type_info& infos);
#endif

  template <class CReleasable, class CClass = CReleasable>
  class Releasable : public CReleasable
  {
  public:
    std::atomic<intptr_t> nref;

    Releasable() : nref(1)
    {
#ifdef _RELEASABLE_DEBUG
      if (_report_releasable_new(typeid(CClass)) >= _RELEASABLE_DEBUG_LEAK_OVERFLOW) {
        printf("(new) %.X : %s\n", this, typeid(CClass).name());
      }
#endif
    }
    virtual void retain() override
    {
      this->nref++;
#ifdef _RELEASABLE_DEBUG
      if (_report_releasable_count(typeid(CClass)) >= _RELEASABLE_DEBUG_LEAK_OVERFLOW) {
        printf("(retain) %.X %d : %s\n", this, this->nref.load(), typeid(CClass).name());
      }
#endif
    }
    virtual void release() override
    {
      if ((--this->nref) <= 0) {
#ifdef _RELEASABLE_DEBUG
        if (this->nref < 0) {
          printf("(!) over delete risk on  %.X : %s\n", this, typeid(CClass).name());
        }
        else {
          if (_report_releasable_delete(typeid(CClass))) {
            printf("<delete> %.X : %s\n", this, typeid(CClass).name());
          }
          this->~Releasable();
        }
#else
        delete this;
#endif
      }
#ifdef _RELEASABLE_DEBUG
      else if (_report_releasable_count(typeid(CClass)) >= _RELEASABLE_DEBUG_LEAK_OVERFLOW) {
        printf("<release> %.X %d : %s\n", this, this->nref.load(), typeid(CClass).name());
      }
#endif
    }
    virtual void __check__alive() override {
      if (this->nref <= 0) {
        printf("(!) dead reference risk on %s\n", typeid(CClass).name());
      }
    }
    virtual ~Releasable() { }
  };
} // namespace webx

#endif
