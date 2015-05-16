
#include "debug/debug.h"

#include "user_config.h"
#include "driver/uart.h"


void debug_init(UartBautRate baud) {
	uart_init(baud, baud);
	os_install_putc1((void*) uart0_putc);
}
