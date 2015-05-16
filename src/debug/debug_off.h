
#include "debug/debug.h"


#undef DEBUG_INIT
#undef DEBUG
#undef DEBUG_PRINTF
#undef DEBUG_STRING
#undef DEBUG_FUNCTION
#undef DEBUG_FUNCTION_START
#undef DEBUG_FUNCTION_END

#define DEBUG_INIT(baud)
#define DEBUG(str, ...)
#define DEBUG_PRINTF(str, ...)
#define DEBUG_STRING(str, len)
#define DEBUG_FUNCTION(pre)
#define DEBUG_FUNCTION_START()
#define DEBUG_FUNCTION_END()
