#include "scheduler.h"

namespace kernel {
Scheduler sched;

void Scheduler::InitThreadQUeue()
{
	ready.InitHead();
	waiting.InitHead();
	exit.InitHead();
}
void Scheduler::AddThreadNode() {

}
}
