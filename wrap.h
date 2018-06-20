#ifndef __WRAP_H__
#define __WRAP_H__
uint64_t
call_aerokernel_func (void (*func)(void), 
		      uint64_t a1,
		      uint64_t a2,
		      uint64_t a3,
		      uint64_t a4,
		      uint64_t a5,
		      uint64_t a6,
		      uint64_t a7,
		      uint64_t a8,
		      char * name);

#endif
