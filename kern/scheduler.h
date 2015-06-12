//-*- c++ -*-

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "thread.h"
#include "libc/list.h"

namespace kernel {

class Scheduler
{
public:
	Scheduler();
	virtual ~Scheduler();
	void InitThreadQUeue();
	//round robin
	void AddThreadNode();
	void DeleteThreadNode();
	Thread *RoundRobin();
	void SwitchToNext(Thread *next_thread);

private:
	struct ThreadNode {
		ListNode list_node;
		Thread *thr;
	};

	ListNode ready, waiting, exit;
};

extern Scheduler sched;

}


#endif /* SCHEDULE_H */
