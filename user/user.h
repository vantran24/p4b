#ifndef _USER_H_
#define _USER_H_


// Mutual exclusion lock.
typedef struct __lock_t {
	uint  locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  unsigned int  pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
}lock_t;

typedef struct __cond_t {
	uint  true;		 //condition true or not
}cond_t;

struct stat;

// system calls

int fork(void);
//added
int clone (void(*fcn)(void*), void *arg, void*stack);
int join (void **stack);
//added
int test(int, int);

int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// user library functions (ulib.c)
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
//added

int thread_create(void (*start_routine)(void*), void *arg);
int thread_join();
void lock_init(struct lock_t *lk);
void lock_acquire(struct lock_t *lk);
void lock_release(struct lock_t *lk);
//void cv_wait(struct cond_t *cv, struct lock_t *lk);
//void cv_signal(struct cond_t *cv);

#endif // _USER_H_

