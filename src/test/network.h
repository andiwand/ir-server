
#ifndef INCLUDE_TEST_NETWORK_H_
#define INCLUDE_TEST_NETWORK_H_


#include "c_types.h"


#define TEST_STATS_INTERVAL 1000


void null_tcp_server_start(uint16 port);
static void null_tcp_server_connect(void* arg);
static void null_tcp_server_discon(void* arg);
static void null_tcp_server_recv(void* arg, char* data, unsigned short length);

void echo_tcp_server_start(uint16 port, uint8 n);
static void echo_tcp_server_connect(void* arg);
static void echo_tcp_server_discon(void* arg);
static void echo_tcp_server_recv(void* arg, char* data, unsigned short length);
static void echo_tcp_server_sent(void* arg);


#endif /* INCLUDE_TEST_NETWORK_H_ */
