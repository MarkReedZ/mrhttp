
// Would this be better?
//#define DBG(x) if (0) { x }
#define DBG if(0) 
#define DBG_PARSER if(0)
#define DBG_RESP if(0)

#define DBG_MEMCAC if(0) 
#define DBG_MRQ if(0) 

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

