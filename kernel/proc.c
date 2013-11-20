#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
	struct proc *p;
	char *sp;//stack pointer

	acquire(&ptable.lock);//lock
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)//running through
		//page table
		if(p->state == UNUSED)
			goto found;
	release(&ptable.lock);//unlock
	return 0;

	found:
	p->state = EMBRYO;
	p->pid = nextpid++;
	release(&ptable.lock);

	// Allocate kernel stack if possible.
	if((p->kstack = kalloc()) == 0){
		p->state = UNUSED;
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;//put the stack pointer at the bottom

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe*)sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint*)sp = (uint)trapret;//

	sp -= sizeof *p->context;//context switch
	p->context = (struct context*)sp;//casting
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint)forkret;

	return p;
}

// Set up first user process.
void
userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];

	p = allocproc();
	acquire(&ptable.lock);
	initproc = p;
	if((p->pgdir = setupkvm()) == 0)
		panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->tf->es = p->tf->ds;
	p->tf->ss = p->tf->ds;
	p->tf->eflags = FL_IF;
	p->tf->esp = PGSIZE;											//init stack ptr
	p->tf->eip = 0;  // beginning of initcode.S					//init instruction ptr

	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	p->state = RUNNABLE;
	release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
	uint sz;

	sz = proc->sz;
	if(n > 0){
		if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
			return -1;
	} else if(n < 0){
		if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
			return -1;
	}
	proc->sz = sz;
	switchuvm(proc);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
	int i, pid;
	struct proc *np;

	// Allocate process.
	if((np = allocproc()) == 0)
		return -1;

	// Copy process state from p.
	if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
		kfree(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		return -1;
	}
	np->sz = proc->sz;
	np->parent = proc;
	*np->tf = *proc->tf;//making full copy of the trap frame
	np->tf->eax = 0;
	for(i = 0; i < NOFILE; i++)
		if(proc->ofile[i])
			np->ofile[i] = filedup(proc->ofile[i]);
	np->cwd = idup(proc->cwd);

	pid = np->pid;
	np->state = RUNNABLE;
	safestrcpy(np->name, proc->name, sizeof(proc->name));
	return pid;
}

int test (int n, int m)
{
	cprintf("this test blows x %d + %d\n", n, m);
	return 0;
}
//before clone is called need to call malloc to make user stack
//then send that pointer to clone
//fcn is the start addr when it returns
int clone(void(*fcn)(void*), void *arg, void *stack)
{
	//checked if stack is page aligned
	if ((uint)stack % PGSIZE != 0) {
		return -1;
	}
	// check stack is in heap
	if ((uint)stack + PGSIZE > proc->sz) {
		return -1;
	}
	int i, pid;
	struct proc *thread;

	// Allocate process.
	if((thread = allocproc()) == 0)//sets up the kstack for thread
		return -1;

	// Copy process state from p.

	//point this to same page directory as parent's not a
	//new one

	//sets addr space same as the parent
	thread->pgdir = proc->pgdir;
	//kfree(thread->kstack);
	//thread->kstack = 0; 		//bottom of the kernel stack
	//thread->state = UNUSED;

	//these should be left the same
	thread->thrstk = stack;
	thread->clonecalled = 1;
	thread->sz = proc->sz;
	thread->parent = proc;
	*thread->tf = *proc->tf;		// making full copy of the trap frame
	//thread->tf->eax = 0;			// eax is in the trap frame so it can return
	// something diff
	// Clear %eax so that fork returns 0 in the child.

	// setup new user stack
	// and registers (np->tf->eip) instruction pt
	// and (np->tf->esp) stack pt)
	uint userstk [2];
	uint usptr;
	usptr = (int) stack + PGSIZE;
	userstk[0] = 0xffffffff;
	userstk[1] = (uint) arg;

	//  stack += PGSIZE; 						//set pointer to the bottom of the stack
	//  	  	  	  	  	  	  	  	  	    //so we can grow it backward
	//  stack -= sizeof (uint);
	//  stack = (void*) 0xffffffff;					//fake addr
	//  stack -= sizeof arg;  				//put on arg on first
	//  stack = arg;
	//stack -= sizeof fcn;					//return addr on next
	//stack = fcn;
	//stack -= sizeof thread->tf->eip;		//instr pointer


	usptr -= 2*sizeof (uint);
	copyout(thread->pgdir, usptr, userstk, 2*sizeof (uint));
	//stack = (void *) thread->tf->eip;
	thread->tf->eip = (uint)fcn;
	//stack -= sizeof thread->tf->esp;		//stack pointer
	//stack = (void *) thread->tf->esp;
	//thread->tf->esp = (uint) stack;

	thread->tf->esp = usptr;
	for(i = 0; i < NOFILE; i++)//NOFILE: num of open files
		if(proc->ofile[i])
			thread->ofile[i] = filedup(proc->ofile[i]);
	thread->cwd = idup(proc->cwd);
	thread->pid = proc->pid;

	thread->state = RUNNABLE;//making the state runnable
	pid = thread->pid;
	//cprint()
	safestrcpy(thread->name, proc->name, sizeof(proc->name));
	return pid;//return of the pid of the new thread is returned to the parent
}

//waits for the child thread that shares the addr with the calling process
//returns:
//PID of waited-child
//or -1 if none
int join (void **stack)
{
	struct proc *p;
	int havekids, pid;

	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for zombie children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			//going through the process table
			if(p->parent != proc || p->clonecalled != 1) //if it is not the parent
				continue;
			havekids = 1;//means proc has kids
			if(p->state == ZOMBIE){//zombie means that it is done running but
				//has not terminated
				// Found one.
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				//freevm(p->pgdir);//frees addr space
				*stack = p->thrstk;
				p->state = UNUSED;
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->clonecalled = 0;
				//pointer put in
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || proc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(proc, &ptable.lock);  //DOC: wait-sleep
	}
	return 0;//change the existing wait()
}


// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
	struct proc *p;
	int fd;

	if(proc == initproc)
		panic("init exiting");

	// Close all open files.
	for(fd = 0; fd < NOFILE; fd++){
		if(proc->ofile[fd]){
			fileclose(proc->ofile[fd]);
			proc->ofile[fd] = 0;
		}
	}

	iput(proc->cwd);
	proc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(proc->parent);

	// Pass abandoned children to init.
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->parent == proc){
			p->parent = initproc;
			if(p->state == ZOMBIE)
				wakeup1(initproc);
		}
	}

	// Jump into the scheduler, never to return.
	proc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
	struct proc *p;
	int havekids, pid;

	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for zombie children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			//going through the process table
			if(p->parent != proc || p->clonecalled == 1) //if it is not the parent
				continue;
			havekids = 1;//means proc has kids
			if(p->state == ZOMBIE){//zombie means that it is done running but
				//has not terminated
				// Found one.
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);//frees addr space
				p->state = UNUSED;
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || proc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(proc, &ptable.lock);  //DOC: wait-sleep
	}
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
	struct proc *p;

	for(;;){
		// Enable interrupts on this processor.
		sti();

		// Loop over process table looking for process to run.
		acquire(&ptable.lock);
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->state != RUNNABLE)
				continue;

			// Switch to chosen process.  It is the process's job
			// to release ptable.lock and then reacquire it
			// before jumping back to us.
			proc = p;
			switchuvm(p);
			p->state = RUNNING;
			swtch(&cpu->scheduler, proc->context);
			switchkvm();

			// Process is done running for now.
			// It should have changed its p->state before coming back.
			proc = 0;
		}
		release(&ptable.lock);

	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
	int intena;

	if(!holding(&ptable.lock))
		panic("sched ptable.lock");
	if(cpu->ncli != 1)
		panic("sched locks");
	if(proc->state == RUNNING)
		panic("sched running");
	if(readeflags()&FL_IF)
		panic("sched interruptible");
	intena = cpu->intena;
	swtch(&proc->context, cpu->scheduler);
	cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock);  //DOC: yieldlock
	proc->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
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
	proc->chan = chan;
	proc->state = SLEEPING;
	sched();

	// Tidy up.
	proc->chan = 0;

	// Reacquire original lock.
	if(lk != &ptable.lock){  //DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
	struct proc *p;

	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->pid == pid){
			p->killed = 1;
			// Wake process from sleep if necessary.
			if(p->state == SLEEPING)
				p->state = RUNNABLE;
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
	static char *states[] = {
			[UNUSED]    "unused",
			[EMBRYO]    "embryo",
			[SLEEPING]  "sleep ",
			[RUNNABLE]  "runble",
			[RUNNING]   "run   ",
			[ZOMBIE]    "zombie"
	};
	int i;
	struct proc *p;
	char *state;
	uint pc[10];

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state == UNUSED)
			continue;
		if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s", p->pid, state, p->name);
		if(p->state == SLEEPING){
			getcallerpcs((uint*)p->context->ebp+2, pc);
			for(i=0; i<10 && pc[i] != 0; i++)
				cprintf(" %p", pc[i]);
		}
		cprintf("\n");
	}
}


