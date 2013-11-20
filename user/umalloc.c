#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "x86.h"

#define PGSIZE (4096)
// Mutual exclusion lock.
struct __lock_t {
	uint  *locked;       	// Is the lock held?
}lock_t;

struct __cond_t {
	uint  true;		 		//condition true or not
}cond_t;

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

int thread_create(void (*start_routine)(void*), void *arg)
{
	// 1)call malloc():to create new user stack
	// 2)call clone(): to create child thread
	void * stack;
	if ((stack = malloc(PGSIZE * 2)) != 0)
	{
		if ((uint)stack % PGSIZE) {
			stack = stack + (4096 - (uint)stack % PGSIZE);
		}
		//stack = malloc(sizeof PGSIZE);
			int child = clone(start_routine, arg, stack);
			return child;
	}
	return NULL;
}
int thread_join()
{
	//how to get the user stack?
	void** stack = malloc(sizeof(void));
	//Call join(): frees the user stack and returns
	int jpid = join(stack);
	free(stack);
	return jpid;
}

//spinlock

void lock_init(struct lock_t *lk)
{
	lk->locked = 0; 	//is it locked?
	//lk->cpu = 0;		//cpu holding lock
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void lock_acquire(struct lock_t *lk)
{
	//pushcli(); // disable interrupts to avoid deadlock.
	//  if(holding(lk))
	//    panic("acquire");

	// The xchg is atomic.
	// It also serializes, so that reads after acquire are not
	// reordered before it.
	while(xchg(&lk->locked, 1) != 0)
		;

	// Record info about lock acquisition for debugging.
	//lk->cpu = cpu;
	//getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void lock_release(struct lock_t *lk)
{
	//  if(!holding(lk))
	//    panic("release");

	//lk->pcs[0] = 0;
	//lk->cpu = 0;

	// The xchg serializes, so that reads before release are
	// not reordered after it.  The 1996 PentiumPro manual (Volume 3,
	// 7.2) says reads can be carried out speculatively and in
	// any order, which implies we need to serialize here.
	// But the 2007 Intel 64 Architecture Memory Ordering White
	// Paper says that Intel 64 and IA-32 will not move a load
	// after a store. So lock->locked = 0 would work here.
	// The xchg being asm volatile ensures gcc emits it after
	// the above assignments (and after the critical section).


	xchg(&lk->locked, 0);

	//popcli();
}
//Condition variables
/*
void cv_wait(struct cond_t *cv, struct lock_t *lk ){
	// 1) put caller to sleep
	 // 2) release the lock

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
	  //sched();

}
void cv_signal(struct cond_t *cv){
	//wake a sleeping thread
	  struct proc *p;
	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	    if(p->state == SLEEPING && p->chan == cv)
	      p->state = RUNNABLE;
}*/
void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}
