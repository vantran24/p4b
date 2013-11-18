#include "types.h"
#include "defs.h"
#include "param.h"//NOFILE is in herr
#include "mmu.h"//PGSIZE is in herrr
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.

//before clone is called need to call malloc to make user stack
//then send that pointer to clone
int
clone(void(*fcn)(void*), void *arg, void*stack)
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
