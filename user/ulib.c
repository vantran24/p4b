#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "x86.h"


//// Mutual exclusion lock.
typedef struct __lock_t{
	volatile uint  locked;       	// Is the lock held?
}lock_t;

typedef struct __cond_t {
	uint  true;		 		//condition true or not
}cont_t;

//spinlock

void lock_init(lock_t *lk)
{
	lk->locked = 0; 	//is it locked?
	//lk->cpu = 0;		//cpu holding lock
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void lock_acquire(lock_t *lk)
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
void lock_release(lock_t *lk)
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

char*
strcpy(char *s, char *t)
{
	char *os;

	os = s;
	while((*s++ = *t++) != 0)
		;
	return os;
}

int
strcmp(const char *p, const char *q)
{
	while(*p && *p == *q)
		p++, q++;
	return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
	int n;

	for(n = 0; s[n]; n++)
		;
	return n;
}

void*
memset(void *dst, int c, uint n)
{
	stosb(dst, c, n);
	return dst;
}

char*
strchr(const char *s, char c)
{
	for(; *s; s++)
		if(*s == c)
			return (char*)s;
	return 0;
}

char*
gets(char *buf, int max)
{
	int i, cc;
	char c;

	for(i=0; i+1 < max; ){
		cc = read(0, &c, 1);
		if(cc < 1)
			break;
		buf[i++] = c;
		if(c == '\n' || c == '\r')
			break;
	}
	buf[i] = '\0';
	return buf;
}

int
stat(char *n, struct stat *st)
{
	int fd;
	int r;

	fd = open(n, O_RDONLY);
	if(fd < 0)
		return -1;
	r = fstat(fd, st);
	close(fd);
	return r;
}

int
atoi(const char *s)
{
	int n;

	n = 0;
	while('0' <= *s && *s <= '9')
		n = n*10 + *s++ - '0';
	return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
	char *dst, *src;

	dst = vdst;
	src = vsrc;
	while(n-- > 0)
		*dst++ = *src++;
	return vdst;
}
