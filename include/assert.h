#include "exception.h"

#undef assert

#ifdef NDEBUG
#define assert(expr, msg)		((void)0)
#else
#define assert(expr, msg)		((expr) ? (void)0 : panic(msg))
#endif