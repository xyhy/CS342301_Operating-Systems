// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "scheduler.h"
#include "copyright.h"
#include "debug.h"
#include "main.h"

//------------------------------
// L1_compare
//------------------------------
static int L1_compare(Thread *x, Thread *y) {
  if (x->ti < y->ti)
    return -1;
  if (x->ti > y->ti)
    return 1;
  if (x->ti == y->ti)
    return 0;
}
//----------------------------------------------------------------
// L2_compare
//----------------------------------------------------------------
static int L2_compare(Thread *x, Thread *y) {
  if (x->priority < y->priority)
    return 1;
  if (x->priority > y->priority)
    return -1;
  return 0;
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler() {
  readyList = new List<Thread *>;
  toBeDestroyed = NULL;

  L1 = new SortedList<Thread *>(L1_compare);
  L2 = new SortedList<Thread *>(L2_compare);
  L3 = new List<Thread *>;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler() {

  delete L1;
  delete L2;
  delete L3;
  delete readyList;
}

//----------------------------------------------------------------------
// Scheduler::AddToQueue
//----------------------------------------------------------------------
void Scheduler::AddToQueue(Thread *thread, int priority) {
  thread->set_start_wait_time(kernel->stats->totalTicks);

  if (priority >= 100) {
    thread->setStatus(READY);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {1}​ ]");
    L1->Insert(thread);
  } else if (priority >= 50) {
    thread->setStatus(READY);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {2}​ ]");
    L2->Insert(thread);
  } else {
    thread->setStatus(READY);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {3}​ ]");
    L3->Append(thread);
  }
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread) {
  ASSERT(kernel->interrupt->getLevel() == IntOff);
  DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
  // cout << "Putting thread on ready list: " << thread->getName() << endl ;
  thread->setStatus(READY);
  //   readyList->Append(thread);
  thread->set_start_wait_time(kernel->stats->totalTicks);

  if (thread->priority >= 100) {
    // thread->setStatus(READY);
    L1->Insert(thread);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {1}​ ]");
  } else if (thread->priority >= 50) {
    // thread->setStatus(READY);
    L2->Insert(thread);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {2}​ ]");
  } else {
    // thread->setStatus(READY);
    L3->Append(thread);
    DEBUG(dbgZ, "[A] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << thread->getID()
                    << "}​ ] is inserted into queueL[​ {3}​ ]");
  }
}

//------------------------------
// Scheduler::Scheduling
//------------------------------
Thread *Scheduler::Scheduling() {
  Thread *next_Thread;

  if (!L1->IsEmpty()) {
    next_Thread = L1->RemoveFront();
    next_Thread->record_start_time(kernel->stats->totalTicks);
    DEBUG(dbgZ, "[B] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is removed from queue L[​ {1}​ ]");
    DEBUG(dbgZ, "[E] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is now selected for execution, thread [​ {"
                    << kernel->currentThread->getID()
                    << "}​ ] is replaced, and it has executed [​ {"
                    << kernel->stats->totalTicks -
                           kernel->currentThread->CPU_start_time
                    << "}​ ] ticks ");
    return next_Thread;
  }

  if (!L2->IsEmpty()) {
    next_Thread = L2->RemoveFront();
    next_Thread->record_start_time(kernel->stats->totalTicks);
    DEBUG(dbgZ, "[B] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is removed from queue L[​ {2}​ ]");
    DEBUG(dbgZ, "[E] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is now selected for execution, thread [​ {"
                    << kernel->currentThread->getID()
                    << "}​ ] is replaced, and it has executed [​ {"
                    << kernel->stats->totalTicks -
                           kernel->currentThread->CPU_start_time
                    << "}​ ] ticks ");
    return next_Thread;
  }

  if (!L3->IsEmpty()) {
    next_Thread = L3->RemoveFront();
    next_Thread->record_start_time(kernel->stats->totalTicks);
    DEBUG(dbgZ, "[B] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is removed from queue L[​ {3}​ ]");
    DEBUG(dbgZ, "[E] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << next_Thread->getID()
                    << "}​ ] is now selected for execution, thread [​ {"
                    << kernel->currentThread->getID()
                    << "}​ ] is replaced, and it has executed [​ {"
                    << kernel->stats->totalTicks -
                           kernel->currentThread->CPU_start_time
                    << "}​ ] ticks ");
    return next_Thread;
  }

  return NULL;
}

//------------------------------
// Scheduler::Aging
//--------------------------------
void Scheduler::Aging() {
  Thread *thread;
  int totalTicks = kernel->stats->totalTicks;

  if (!L1->IsEmpty()) {
    ListIterator<Thread *> *iterator;
    iterator = new ListIterator<Thread *>(L1);
    while (!iterator->IsDone()) {
      thread = iterator->Item();
      thread->ready_queue_wait_time += totalTicks - thread->enter_ready_time;
      thread->enter_ready_time = totalTicks;

      if (thread->ready_queue_wait_time >= 1500) {
        if (thread->priority + 10 >= 149) {
          DEBUG(dbgZ, "[C] Tick [​ {"
                          << kernel->stats->totalTicks
                          << "}​ ]: Thread [​ {"
                          << iterator->Item()->getID()
                          << "}​ ] changes its priority from [​ {"
                          << iterator->Item()->priority
                          << "}​ ] to [​ {149}​ ]");
          thread->priority = 149;
        } else {
          DEBUG(dbgZ, "[C] Tick [​ {"
                          << kernel->stats->totalTicks
                          << "}​ ]: Thread [​ {"
                          << iterator->Item()->getID()
                          << "}​ ] changes its priority from [​ {"
                          << iterator->Item()->priority << "}​ ] to [​ {"
                          << iterator->Item()->priority + 10 << "}​ ]");
          thread->priority += 10;
        }
        thread->ready_queue_wait_time -= 1500;
      }
      iterator->Next();
    }
  }

  if (!L2->IsEmpty()) {
    ListIterator<Thread *> *iterator;
    iterator = new ListIterator<Thread *>(L2);
    while (!iterator->IsDone()) {
      thread = iterator->Item();
      thread->ready_queue_wait_time += totalTicks - thread->enter_ready_time;
      thread->enter_ready_time = totalTicks;

      if (thread->ready_queue_wait_time >= 1500) {
        DEBUG(dbgZ, "[C] Tick [​ {"
                        << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                        << iterator->Item()->getID()
                        << "}​ ] changes its priority from [​ {"
                        << iterator->Item()->priority << "}​ ] to [​ {"
                        << iterator->Item()->priority + 10 << "}​ ]");
        thread->priority += 10;
        thread->ready_queue_wait_time -= 1500;
      }
      iterator->Next();
    }
  }

  if (!L3->IsEmpty()) {
    ListIterator<Thread *> *iterator;
    iterator = new ListIterator<Thread *>(L3);
    while (!iterator->IsDone()) {
      thread = iterator->Item();
      thread->ready_queue_wait_time += totalTicks - thread->enter_ready_time;
      thread->enter_ready_time = totalTicks;

      if (thread->ready_queue_wait_time >= 1500) {
        thread->priority += 10;
        thread->ready_queue_wait_time -= 1500;
      }
      iterator->Next();
    }
  }
}

//------------------------------
// Scheduler::ReArrangeThreads
//--------------------------------------------------------
void Scheduler::ReArrangeThreads() {
  Thread *move_thread;
  ListIterator<Thread *> *iter3; // L3
  iter3 = new ListIterator<Thread *>(L3);
  while (!iter3->IsDone()) {
    move_thread = L3->RemoveFront();
    DEBUG(dbgZ, "[B] Tick [​ {"
                    << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                    << move_thread->getID()
                    << "}​ ] is removed from queue L[​ {3}​ ]");
    if (move_thread->priority >= 100) {
      L1->Insert(move_thread);
      DEBUG(dbgZ, "[A] Tick [​ {"
                      << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                      << move_thread->getID()
                      << "}​ ] is inserted into queue L[​ {1}​ ]");
      this->CheckPreempt(move_thread);
    } else if (move_thread->priority >= 50) {
      L2->Insert(move_thread);
      DEBUG(dbgZ, "[A] Tick [​ {"
                      << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                      << move_thread->getID()
                      << "}​ ] is inserted into queue L[​ {2}​ ]");
      this->CheckPreempt(move_thread);
    } else {
      L3->Append(move_thread);
      DEBUG(dbgZ, "[A] Tick [​ {"
                      << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                      << move_thread->getID()
                      << "}​ ] is inserted into queue L[​ {3}​ ]");
    }

    iter3->Next();
  }

  ListIterator<Thread *> *iter2; // L2
  iter2 = new ListIterator<Thread *>(L2);
  while (!iter2->IsDone()) {
    if (iter2->Item()->priority >= 100) {
      move_thread = L2->RemoveFront();
      DEBUG(dbgZ, "[B] Tick [​ {"
                      << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                      << move_thread->getID()
                      << "}​ ] is removed from queue L[​ {2}​ ]");
      L1->Insert(move_thread);
      DEBUG(dbgZ, "[A] Tick [​ {"
                      << kernel->stats->totalTicks << "}​ ]: Thread [​ {"
                      << move_thread->getID()
                      << "}​ ] is inserted into queue L[​ {1}​ ]");
      this->CheckPreempt(move_thread);
    }
    iter2->Next();
  }
}

//------------------------------
// Scheduler::CheckPreempt
//----------------------------------------------------------------

void Scheduler::CheckPreempt(Thread *thread) {
  // current thread is L3 and L1/L2 has thread added.
  if (kernel->currentThread->InWhichQueue() == 3 &&
      (thread->InWhichQueue() == 2 || thread->InWhichQueue() == 1)) {
    kernel->currentThread->T +=
        kernel->stats->totalTicks - kernel->currentThread->CPU_start_time;
    Thread *nextThread = this->Scheduling();
    // this->AddToQueue(kernel->currentThread, kernel->currentThread->priority);
    this->ReadyToRun(kernel->currentThread);
    this->Run(nextThread, FALSE);
  } else if (kernel->currentThread->InWhichQueue() == 2 &&
             thread->InWhichQueue() == 1) {
    kernel->currentThread->T +=
        kernel->stats->totalTicks - kernel->currentThread->CPU_start_time;
    Thread *nextThread = this->Scheduling();
    // this->AddToQueue(kernel->currentThread, kernel->currentThread->priority);
    this->ReadyToRun(kernel->currentThread);
    this->Run(nextThread, FALSE);
  } else if (kernel->currentThread->InWhichQueue() == 1 &&
             (thread->InWhichQueue() == 1 &&
              thread->ti < kernel->currentThread->ti)) {
    kernel->currentThread->T +=
        kernel->stats->totalTicks - kernel->currentThread->CPU_start_time;
    Thread *nextThread = this->Scheduling();
    // this->AddToQueue(kernel->currentThread, kernel->currentThread->priority);
    this->ReadyToRun(kernel->currentThread);
    this->Run(nextThread, FALSE);
  }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *Scheduler::FindNextToRun() {
  ASSERT(kernel->interrupt->getLevel() == IntOff);

  if (readyList->IsEmpty()) {
    return NULL;
  } else {
    return readyList->RemoveFront();
  }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing) {
  Thread *oldThread = kernel->currentThread;

  ASSERT(kernel->interrupt->getLevel() == IntOff);

  if (finishing) { // mark that we need to delete current thread
    ASSERT(toBeDestroyed == NULL);
    toBeDestroyed = oldThread;
  }

  if (oldThread->space != NULL) { // if this thread is a user program,
    oldThread->SaveUserState();   // save the user's CPU registers
    oldThread->space->SaveState();
  }

  oldThread->CheckOverflow(); // check if the old thread
                              // had an undetected stack overflow

  kernel->currentThread = nextThread; // switch to the next thread
  nextThread->setStatus(RUNNING);     // nextThread is now running

  DEBUG(dbgThread, "Switching from: " << oldThread->getName()
                                      << " to: " << nextThread->getName());

  // This is a machine-dependent assembly language routine defined
  // in switch.s.  You may have to think
  // a bit to figure out what happens after this, both from the point
  // of view of the thread and from the perspective of the "outside world".

  nextThread->ready_queue_wait_time = 0;

  SWITCH(oldThread, nextThread);

  // we're back, running oldThread

  // interrupts are off when we return from switch!
  ASSERT(kernel->interrupt->getLevel() == IntOff);

  DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

  CheckToBeDestroyed(); // check if thread we were running
                        // before this one has finished
                        // and needs to be cleaned up

  if (oldThread->space != NULL) {  // if there is an address space
    oldThread->RestoreUserState(); // to restore, do it.
    oldThread->space->RestoreState();
  }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void Scheduler::CheckToBeDestroyed() {
  if (toBeDestroyed != NULL) {
    delete toBeDestroyed;
    toBeDestroyed = NULL;
  }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print() {
  cout << "Ready list contents:\n";
  readyList->Apply(ThreadPrint);
}
