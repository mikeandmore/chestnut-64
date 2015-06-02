//-*- c++ -*-

#ifndef THREAD_H
#define THREAD_H

#include "console.h"
#include "mm/allocator.h"

namespace kernel {
class Thread
{
public:
	Thread();
	virtual ~Thread();
	u64 Create();
     	void Yield();
	void Destroy();

protected:
	u64 thread_id;
	u64 priority;
	Page *stack;
	struct {
		//registers:
		//
		u64 reg[18];
	} context;
};

}


#endif /* THREAD_H */
