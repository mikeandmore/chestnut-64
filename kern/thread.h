//-*- c++ -*-

#ifndef THREAD_H
#define THREAD_H

#include "console.h"
#include "libc/common.h"
#include "mm/allocator.h"

namespace kernel {

struct Context {
  u64 rax;
  u64 rbx;
  u64 rcx;
  u64 rdx;
  u64 rdi;
  u64 rsi;
  u64 rbp;
  u64 rsp;

  u64 r8;
  u64 r9;
  u64 r10;
  u64 r11;
  u64 r12;
  u64 r13;
  u64 r14;
  u64 r15;

  u64 rip;

  u64 fp_state[64];
};

class Thread
{
public:
  Thread();
  ~Thread();

  void Yield();
  void Sleep(int second);

  bool  __no_inline SaveContext();
  void RestoreContext() __no_return;

  virtual void Run() = 0;

  void Start();
  bool is_initialized() const { return is_initialized_; }
private:
  void Stub();
protected:
  u64 thread_id;
  u64 priority;

  Context ctx;

  enum {
    Ready, Running, Blocked, Exited
  } states;

  bool is_initialized_;
};

}


#endif /* THREAD_H */
