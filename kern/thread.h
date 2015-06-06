//-*- c++ -*-

#ifndef THREAD_H
#define THREAD_H

#include "console.h"
#include "libc/common.h"
#include "mm/allocator.h"

namespace kernel {

enum RegisterIndex {
	kRegRAX = 0,
	kRegRBX,
	kRegRBP,
	kRegR12,
	kRegR13,
	kRegR14,
	kRegR15,
	kRegRDI,

	kRegRSI,
	kRegRDX,
	kRegRCX,
	kRegR8,
	kRegR9,

	kRegRIP,
	kRegRSP,

	kNumReg, // total number of registers
};

class Thread
{
public:
	Thread();

	/**
	 * Questions:
	 *
	 * 1. You can't kill another thread. Why?
	 * 2. When the thread routine wants to exit the current thread, you
	 * cannot destroy the current thread right there. Why?
	 * 3. Therefore, with the two questions above, why there is a Stub()
	 * method?
	 */

	void Yield();

	/**
	 * These two functions are the magic.
	 *
	 * SaveContext() will save the state of execution to as if the function
	 * has returned. As the X86_64 calling convention specified, %RAX holds
	 * the return value. After calling this function, it returns a true
	 * (%RAX=1), which means, the scheduler need to suspend this thread
	 * ASAP.
	 *
	 * RestoreContext() could restore the state SaveContext() saved, and
	 * plus, change the %RAX to zero. Since SaveContext() saves the state
	 * as if it was fininshed, we will jump back to the state it has just
	 * returned. However, this time, with %RAX=0, which means, the thread
	 * should continue to execute.
	 *
	 * Combine these two method, we could use the following pattern to yield
	 * a thread:
	 *
	 * if (SaveContext()) SchedulerStopMe();
	 *
	 */

	void Sleep(int second);
	bool  __no_inline SaveContext();
	void RestoreContext() __no_return;

	virtual void Run() {} // should be = 0 later...
	void Start();

	bool is_initialized() const { return is_initialized_; }

private:
	void Stub();

protected:
	u64 thread_id;
	u64 priority;

	struct {
		// registers:
		u64 reg[kNumReg];
		Page *stack;
	} context;

	enum {
		Ready, Running, Blocked, Exited
	} states;

	bool is_initialized_;
};

}


#endif /* THREAD_H */