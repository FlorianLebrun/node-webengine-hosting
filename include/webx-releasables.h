
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
class Releasable : public CReleasable
{
  public:
  std::atomic<intptr_t> nref;

  Releasable() : nref(1)
  {
  }
  virtual void retain() override
  {
    this->nref++;
  }
  virtual void release() override
  {
    if ((--this->nref) == 0) this->free();
  }
  virtual void free() = 0;
};
} // namespace webx

#endif
