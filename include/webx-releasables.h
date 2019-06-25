
#ifndef webx_engine_objects_h_
#define webx_engine_objects_h_

#include <map>
#include <stdint.h>
#include <string>
#include <atomic>

#ifdef _DEBUG
#define _RELEASABLE_DEBUG
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
    CReleasable* flush() {
      CReleasable* object = this->_object;
      this->_object = 0;
      return object;
    }
    CReleasable* operator = (CReleasable* object) {
      if (this->_object) this->_object->release();
      if (object) object->retain();
      return this->_object = object;
    }
    CReleasable* operator -> () {
      return this->_object;
    }
    CReleasable* operator * () {
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
      if (_report_releasable_new(typeid(CClass)) > 10) {
        printf("(new) %.X : %s\n", this, typeid(CClass).name());
      }
#endif
    }
    virtual void retain() override
    {
      this->nref++;
#ifdef _RELEASABLE_DEBUG
      if (_report_releasable_count(typeid(CClass)) > 10) {
        printf("(retain) %.X %d : %s\n", this, this->nref.load(), typeid(CClass).name());
      }
#endif
    }
    virtual void release() override
    {
      if ((--this->nref) <= 0) {
#ifdef _RELEASABLE_DEBUG
        _report_releasable_delete(typeid(CClass));
        if (this->nref < 0) {
          printf("(!) crash risk on %s\n", typeid(CClass).name());
        }
        this->~Releasable();
#else
        delete this;
#endif
      }
#ifdef _RELEASABLE_DEBUG
      if (_report_releasable_count(typeid(CClass)) > 10) {
        if(this->nref) printf("<release> %.X %d : %s\n", this, this->nref.load(), typeid(CClass).name());
        else printf("<delete> %.X : %s\n", this, this->nref.load(), typeid(CClass).name());
      }
#endif
    }
    virtual ~Releasable() {}
  };
} // namespace webx

#endif
