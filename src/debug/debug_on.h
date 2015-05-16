
#include "debug/debug.h"


#undef DEBUG_INIT
#undef DEBUG
#undef DEBUG_PRINTF
#undef DEBUG_STRING
#undef DEBUG_FUNCTION
#undef DEBUG_FUNCTION_START
#undef DEBUG_FUNCTION_END

#define DEBUG_INIT(baud) debug_init(baud)
#define DEBUG(str, ...) do { os_printf(str, ##__VA_ARGS__); os_printf("\n"); } while(false)
#define DEBUG_PRINTF(str, ...) os_printf(str, ##__VA_ARGS__)
#define DEBUG_STRING(str, len) uart0_tx_buffer(str, len)
#define DEBUG_FUNCTION(text) DEBUG("%s: \"%s\" %s:%d", text, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define DEBUG_FUNCTION_START() DEBUG_FUNCTION("start")
#define DEBUG_FUNCTION_END() DEBUG_FUNCTION("end")
