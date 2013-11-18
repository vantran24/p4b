/*
 * thread.h
 *
 *  Created on: Nov 16, 2013
 *      Author: Van
 */

#ifndef THREAD_H_
#define THREAD_H_

// Mutual exclusion lock.
struct lock_t {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uint pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
};

struct cond_t {
	uint true;		 //condition true or not
};

void *thread_create(void (*start_routine)(void*), void *arg);
void thread_join();
void initlock(struct lock_t *lk);
void lock_acquire(struct lock_t *lk);
void lock_release(struct lock_t *lk);
void cv_wait(struct cond_t *cv, struct lock_t *lk);
void cv_signal(struct cond_t *cv);

#endif /* THREAD_H_ */
