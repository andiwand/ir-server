
#include "network/beacon.h"

#include "osapi.h"

#include "util.h"
#include "network/socket.h"

#include "debug/debug_on.h"


static void beacon_send(void* arg);
static void beacon_sent(socket_t* client);


beacon_t* ICACHE_FLASH_ATTR beacon_create(beacon_t* beacon, uint16_t port) {
	DEBUG_FUNCTION_START();

	if (beacon == NULL) beacon = (beacon_t*) m_malloc(sizeof(beacon_t));

	uint32_t ip = BEACON_ADDRESS;
	socket_create_udp(&beacon->socket, &ip, port);

	beacon->socket.sent_cb = beacon_sent;
	beacon->socket.reverse = beacon;

	beacon->send_cb = NULL;

	return beacon;
}

void ICACHE_FLASH_ATTR beacon_destroy(beacon_t* beacon, bool all) {
	if (all) m_free(beacon);
}

void ICACHE_FLASH_ATTR beacon_start(beacon_t* beacon) {
	DEBUG_FUNCTION_START();

	if (beacon->running) return;

	os_timer_disarm(&beacon->timer);
	os_timer_setfn(&beacon->timer, beacon_send, beacon);
	os_timer_arm(&beacon->timer, beacon->interval, false);

	beacon->running = true;
}

void ICACHE_FLASH_ATTR beacon_stop(beacon_t* beacon) {
	if (!beacon->running) return;

	os_timer_disarm(&beacon->timer);

	beacon->running = false;
}

static void ICACHE_FLASH_ATTR beacon_send(void* arg) {
	beacon_t* beacon = (beacon_t*) arg;
	if (!beacon->running) return;

	if (beacon->send_cb != NULL) beacon->send_cb(beacon);

	if (beacon->data.start == NULL) return;
	if (beacon->data.length <= 0) return;

	socket_send(&beacon->socket, beacon->data.start, beacon->data.length);
}

static void beacon_sent(socket_t* client) {
	beacon_t* beacon = (beacon_t*) client->reverse;

	os_timer_arm(&beacon->timer, beacon->interval, false);
}
