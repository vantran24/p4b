#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
// Mutual exclusion lock.
//struct __lock_t{
//	volatile uint locked;       	// Is the lock held?
//}lock_t;
//
//struct __cond_t{
//	uint true;		 		//condition true or not
//}cond_t;


#ifndef NULL
#define NULL (0)
#endif

#endif //_TYPES_H_
