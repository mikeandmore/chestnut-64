//-*- c++ -*-

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "thread.h"
#include "libc/list.h"
#include "mm/allocator.h"

namespace kernel {

class Scheduler
{
	struct ThreadNode {
		ListNode list_node;
		Thread *thr;
	};
public:
	Scheduler();
	virtual ~Scheduler();
	void InitThreadQUeue();
	//round robin
	void AddThread(Thread *thr);
	void ExitCurrentThread();
	ThreadNode *RoundRobin();
	void SwitchToNext();

private:

<<<<<<< HEAD
	ListNode ready, waiting, exit;
	ThreadNode *cur_thread;
=======
	ThreadNode ready, waiting, exit;
	ListNode ready_head, waiting_head, exit_head;
>>>>>>> adding acpica
};

extern Scheduler sched;

}


#endif /* SCHEDULE_H */
