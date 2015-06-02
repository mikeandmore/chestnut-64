//-*- c++ -*-

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "thread.h"
#include "libc/list.h"

namespace kernel {
class Schedule
{
public:
	Schedule();
	virtual ~Schedule();
	//round robin
	void AddThreadNode();
	void DeleteThreadNode();
	Thread *RoundRobin();
	void SwitchToNext(Thread *next_thread);

private:
	struct ThreadNode {
		ListNode list_node;
		Thread *current;
	};

	ListNode head;
};

}


#endif /* SCHEDULE_H */
