
#ifndef BASE_SPINLOCK_H_
#define BASE_SPINLOCK_H_

#include <stdint.h>
#include <atomic>
#include <intrin.h>

typedef int tSpinWord;

//#define DISABLE_LOCK

class SpinLock {
public:

   SpinLock() : lockword_(kSpinLockFree) {
   }

   inline void lock() {
#ifndef DISABLE_LOCK
      tSpinWord lockValue = kSpinLockFree;
      if(!lockword_.compare_exchange_weak(lockValue, kSpinLockHeld)) {
         slowLock();
      }
#endif
   }

   inline bool tryLock() {
#ifndef DISABLE_LOCK
      tSpinWord lockValue = kSpinLockFree;
      return lockword_.compare_exchange_weak(lockValue, kSpinLockHeld);
#else
      return true;
#endif
   }

   inline void unlock() {
#ifndef DISABLE_LOCK
      uint64_t prev_value = static_cast<uint64_t>(lockword_.exchange(kSpinLockFree));
      if (prev_value != kSpinLockHeld) slowUnlock();
#endif
   }

   inline bool isHeld() const {
#ifndef DISABLE_LOCK
      return lockword_.load() != kSpinLockFree;
#else
      return false;
#endif
   }

   inline static void SpinlockPause() {
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
      __asm__ __volatile__("rep; nop" : : );
#else
      __nop();
#endif
   }

   static void SpinlockWait(volatile tSpinWord *at, int32_t value, int loop);
   static void SpinlockWake(volatile tSpinWord *at);

private:
   enum { 
      kSpinLockFree = 0,
      kSpinLockHeld = 1,
      kSpinLockSleeper = 2 
   };

   std::atomic<tSpinWord> lockword_;

   void slowLock();
   void slowUnlock();
};

#endif  // BASE_SPINLOCK_H_
