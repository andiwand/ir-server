
#ifndef NETWORK_BEACON_H_
#define NETWORK_BEACON_H_


#include "c_types.h"
#include "os_type.h"

#include "util.h"
#include "network/socket.h"


#define BEACON_ADDRESS 0xFFFFFFFF


typedef struct beacon beacon_t;

typedef void (*beacon_send_cb_t) (beacon_t* beacon);


struct beacon {
	socket_t socket;
	os_timer_t timer;
	uint16_t interval;

	bool running;
	buffer_t data;

	beacon_send_cb_t send_cb;
	void* reverse;
};


beacon_t* beacon_create(beacon_t* beacon, uint16_t port);
void beacon_destroy(beacon_t* beacon, bool all);
void beacon_start(beacon_t* beacon);
void beacon_stop(beacon_t* beacon);


#endif /* NETWORK_BEACON_H_ */
