#include "scheduler.h"

namespace kernel {
Scheduler sched;

void Scheduler::InitThreadQUeue()
{
  //change needed
  ready.ready_head.InitHead();
  waiting.waiting_head.InitHead();
  exit.exit_head.InitHead();
}

void Scheduler::AddThread(Thread *thr)
{
  ThreadNode *thr_node = (ThreadNode*) Alloc(sizeof(ThreadNode));
  thr_node->thr = thr;
  thr_node->list_node.InsertBefore(&ready);
}

void Scheduler::ExitCurrentThread()
{
  cur_thread->list_node.InsertAfter(&exit);
  cur_thread = NULL;
  cur_thread = RoundRobin();
  SwitchToNext();
  // TODO: need to free the stack later

}

Scheduler::ThreadNode *Scheduler::RoundRobin()
{
  if (cur_thread != NULL) {
    cur_thread->list_node.InsertBefore(&ready);
  }

  if (ready.is_empty()) {
    return NULL;
  }
  cur_thread = (ThreadNode *)ready.next;
  cur_thread->list_node.Delete();
  return cur_thread;
}

void Scheduler::SwitchToNext()
{
  // TODO: need to switch to cur_thread
  if (cur_thread->thr->is_initialized() == false) {
    cur_thread->thr->Stub();
  } else {
    cur_thread->thr->RestoreContext();
  }
}

}
