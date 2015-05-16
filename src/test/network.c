
#include "test/network.h"

#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"

#include "util.h"
#include "debug/debug.h"


static os_timer_t timer;
static uint32 recv_bytes;
static uint32 recv_count;
static uint32 send_bytes;
static uint32 send_count;
static uint32 sent_count;
static uint32 last_time;

static void stats_reset() {
	recv_bytes = 0;
	recv_count = 0;
	send_bytes = 0;
	send_count = 0;
	sent_count = 0;
	last_time = system_get_time();
}

static void stats_print() {
	uint32 now = system_get_time();
	double time = (now - last_time) / 1000000.0;
	double recv_speed = recv_bytes / time;
	double send_speed = send_bytes / time;
	unsigned int time_ms = time * 1000;
	unsigned int recv_speed_kbps = recv_speed / 1000;
	unsigned int send_speed_kbps = send_speed / 1000;

	DEBUG("recv: %d kb/s (%d pkts); sent: %d kb/s (%d/%d pkts); time: %d ms",
			recv_speed_kbps, recv_count, send_speed_kbps, send_count, sent_count, time_ms);

	stats_reset();
	last_time = now;
}

static void stats_init() {
	stats_reset();

	os_timer_disarm(&timer);
	os_timer_setfn(&timer, stats_print);
	os_timer_arm(&timer, TEST_STATS_INTERVAL, true);
}

static void stats_destroy() {
	os_timer_disarm(&timer);
}

static void stats_recv(uint16 length) {
	recv_bytes += length;
	recv_count++;
}

static void stats_send(uint16 length) {
	send_bytes += length;
	send_count++;
}

static void stats_sent() {
	sent_count++;
}

void null_tcp_server_start(uint16 port) {
	struct espconn* socket = (struct espconn*) os_zalloc(sizeof(struct espconn));
	ets_memset(socket, 0, sizeof(struct espconn));
	socket->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
	ets_memset(socket->proto.tcp, 0, sizeof(esp_tcp));

	socket->type = ESPCONN_TCP;
	socket->proto.tcp->local_port = port;
	espconn_regist_connectcb(socket, null_tcp_server_connect);
	espconn_regist_disconcb(socket, null_tcp_server_discon);
	espconn_create(socket);
	espconn_accept(socket);
}

static void null_tcp_server_connect(void* arg) {
	struct espconn* client = arg;
	stats_init();
	espconn_regist_recvcb(client, null_tcp_server_recv);
}

static void null_tcp_server_discon(void* arg) {
	struct espconn* client = arg;
	stats_destroy();
}

static void null_tcp_server_recv(void* arg, char* data, unsigned short length) {
	struct espconn* client = arg;
	stats_recv(length);
}

static uint8 repeat;
static uint8* buffer;
static uint16 buffer_length;

void echo_tcp_server_start(uint16 port, uint8 n) {
	struct espconn* socket = (struct espconn*) os_zalloc(sizeof(struct espconn));
	ets_memset(socket, 0, sizeof(struct espconn));
	socket->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
	ets_memset(socket->proto.tcp, 0, sizeof(esp_tcp));

	socket->type = ESPCONN_TCP;
	socket->proto.tcp->local_port = port;
	espconn_regist_connectcb(socket, echo_tcp_server_connect);
	espconn_regist_disconcb(socket, echo_tcp_server_discon);

	repeat = n;
	buffer_length = 10000;
	buffer = (uint8*) os_zalloc(buffer_length);

	espconn_create(socket);
	espconn_accept(socket);
}

static void echo_tcp_server_connect(void* arg) {
	struct espconn* client = arg;
	stats_init();
	espconn_regist_recvcb(client, echo_tcp_server_recv);
	espconn_regist_sentcb(client, echo_tcp_server_sent);
}

static void echo_tcp_server_discon(void* arg) {
	struct espconn* client = arg;
	stats_destroy();
}

static void echo_tcp_server_recv(void* arg, char* data, unsigned short length) {
	struct espconn* client = arg;
	stats_recv(length);

	uint16 send_length = length * repeat;
	if (buffer_length < send_length) {
		DEBUG("buffer overflow");
		return;
	}

	uint8 i = 0;
	while (i < repeat) {
		os_memcpy(buffer + i * length, data, length);
		i++;
	}
	espconn_sent(client, buffer, send_length);
	stats_send(send_length);
}

static void echo_tcp_server_sent(void* arg) {
	struct espconn* client = arg;
	stats_sent();
}
