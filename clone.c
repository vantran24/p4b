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
  if((np = allocproc()) == 0)//sets up the kstack for thread
    return -1;

  // Copy process state from p.

  //point this to same page directory as parent's not a
  //new one

  // need to change this
  // we want the same addr space not a copy
  //if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
  if((np->pgdir = proc->pgdir) == 0){
    kfree(thread->kstack);
    thread->kstack = 0;
    thread->state = UNUSED;
    return -1;
  }
  //these should be left the same
  thread->sz = proc->sz;
  thread->parent = proc;
  *thread->tf = *proc->tf;		//making full copy of the trap frame
  thread->tf->eax = 0;			//eax is in the trap frame so it can return something diff
  	  	  	  	  	  	  		// Clear %eax so that fork returns 0 in the child.
  	  	  	  	  	  	  		// pretending this is clone not fork
  	  	  	  	  	  	  		//setup new user stack
  	  	  	  	  	  	  		// and registers (np->tf->eip) instruction pt
  	  	  	  	  	  	  		// and (npt->tf->esp) stck pt)

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
    	thread->ofile[i] = filedup(proc->ofile[i]);
  thread->cwd = idup(proc->cwd);

  pid = thread->pid;
  thread->state = RUNNABLE;//making the state runnable
  safestrcpy(thread->name, proc->name, sizeof(proc->name));
  return pid;//return of the pid of the new thread is returned to the parent
}
