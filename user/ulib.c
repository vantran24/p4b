#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "x86.h"


//spinlock

void lock_init(lock_t *lk)
{
	lk->locked = 0; 	//is it locked?
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void lock_acquire(lock_t *lk)
{
	while(xchg(&lk->locked, 1) != 0)
		;
}

// Release the lock.
void lock_release(lock_t *lk)
{
	xchg(&lk->locked, 0);
}
//Condition variables

void cv_wait(cond_t *cv, lock_t *lk ){
	// 1) put caller to sleep
	 // 2) release the lock
	csleep(cv, lk);
}
void cv_signal(cond_t *cv){
	//wake a sleeping thread
	wakeup1(cv);
}

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
