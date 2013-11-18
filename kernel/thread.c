/*
 * thread.c
 *
 *  Created on: Nov 16, 2013
 *      Author: Van
 */
#include "types.h"
#include "defs.h"
#include "param.h"//NOFILE is in herr
#include "mmu.h"//PGSIZE is in herrr
#include "x86.h"
#include "proc.h"
#include "thread.h"

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.

//before clone is called need to call malloc to make user stack
//then send that pointer to clone
int clone(void(*fcn)(void*), void *arg, void *stack)
{
  int i, pid;
  struct proc *thread;

  // Allocate process.
  if((thread = allocproc()) == 0)//sets up the kstack for thread
    return -1;

  // Copy process state from p.

  //point this to same page directory as parent's not a
  //new one

  // need to change this
  // we want the same addr space not a copy
  //if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
  if((thread->pgdir = proc->pgdir) == 0){//sets addr space same as the
	  	  	  	  	  	  	  	  	  	 //parent
    kfree(thread->kstack);
    thread->kstack = 0; 		//bottom of the kernel stack
    thread->state = UNUSED;
    return -1;
  }
  //these should be left the same
  thread->sz = proc->sz;
  thread->parent = proc;
  *thread->tf = *proc->tf;		// making full copy of the trap frame
  thread->tf->eax = 0;			// eax is in the trap frame so it can return
  	  	  	  	  	  	  	  	// something diff
  	  	  	  	  	  	  		// Clear %eax so that fork returns 0 in the child.

 // setup new user stack
 // and registers (np->tf->eip) instruction pt
 // and (np->tf->esp) stack pt)
  stack += PGSIZE; 						//set pointer to the bottom of the stack
  	  	  	  	  	    				//so we can grow it backward
  stack -= sizeof arg;  				//put on arg on first
  arg = stack;
  stack -= sizeof fcn;					//return addr on next
  fcn = stack;
  stack -= sizeof thread->tf->eip;		//instr pointer
  thread->tf->eip = stack;
  stack -= sizeof thread->tf->esp;		//stack pointer
  thread->tf->esp = stack;


  for(i = 0; i < NOFILE; i++)//NOFILE: num of open files
    if(proc->ofile[i])
    	thread->ofile[i] = filedup(proc->ofile[i]);
  thread->cwd = idup(proc->cwd);

  pid = thread->pid;
  thread->state = RUNNABLE;//making the state runnable
  safestrcpy(thread->name, proc->name, sizeof(proc->name));
  return pid;//return of the pid of the new thread is returned to the parent
}

//waits for the child thread that shares the addr with the calling process
//returns:
//PID of waited-child
//or -1 if none
int join (void **stack)
{
	return wait();//change the existing wait()
}

void *thread_create(void (*start_routine)(void*), void *arg)
{
	/* 1)call malloc():to create new user stack
	 * 2)call clone(): to create child thread
	 */
	void* stack = malloc(PGSIZE);
	int child = clone(start_routine, arg, stack);
	return NULL;
}
void thread_join()
{
	//Call join(): frees the user stack and returns
	join();
}

/*spinlock
 *
 */
void initlock(struct lock_t *lk)
{
  lk->locked = 0; 	//is it locked?
  lk->cpu = 0;		//cpu holding lock
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void lock_acquire(struct lock_t *lk)
{
  pushcli(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

  // The xchg is atomic.
  // It also serializes, so that reads after acquire are not
  // reordered before it.
  while(xchg(&lk->locked, 1) != 0)
    ;

  // Record info about lock acquisition for debugging.
  lk->cpu = cpu;
  getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void lock_release(struct lock_t *lk)
{
  if(!holding(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;

  /*
   *
  // The xchg serializes, so that reads before release are
  // not reordered after it.  The 1996 PentiumPro manual (Volume 3,
  // 7.2) says reads can be carried out speculatively and in
  // any order, which implies we need to serialize here.
  // But the 2007 Intel 64 Architecture Memory Ordering White
  // Paper says that Intel 64 and IA-32 will not move a load
  // after a store. So lock->locked = 0 would work here.
  // The xchg being asm volatile ensures gcc emits it after
  // the above assignments (and after the critical section).
   *
   */
  xchg(&lk->locked, 0);

  popcli();
}
/*Condition variables
 */
void cv_wait(struct cond_t *cv, struct lock_t *lk ){
	/* 1) put caller to sleep
	 * 2) release the lock
	 */
	 if(proc == 0)
	    panic("sleep");

	  if(lk == 0)
	    panic("sleep without lk");

	  // Must acquire ptable.lock in order to
	  // change p->state and then call sched.
	  // Once we hold ptable.lock, we can be
	  // guaranteed that we won't miss any wakeup
	  // (wakeup runs with ptable.lock locked),
	  // so it's okay to release lk.
	  if(lk != &ptable.lock){  //DOC: sleeplock0
	    acquire(&ptable.lock);  //DOC: sleeplock1
	    release(lk);
	  }

	  // Go to sleep.
	  proc->chan = cv;
	  proc->state = SLEEPING;
	  sched();

}
void cv_signal(struct cond_t *cv){
	//wake a sleeping thread
	  struct proc *p;

	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	    if(p->state == SLEEPING && p->chan == cv)
	      p->state = RUNNABLE;
}



