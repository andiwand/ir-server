
#include "network/socket.h"

#include "user_interface.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "espconn.h"

#include "util.h"

#include "debug/debug_on.h"


static int_table_t* server_table();

static void connect(void* arg);
static void receive(void* arg, char* data, uint16_t length);
static void sent(void* arg);
static void error(void* arg, int8_t error);
static void disconnect(void* arg);


static int_table_t* server_table() {
	static int_table_t* table;
	if (table == NULL) table = int_table_create(SERVER_TABLE_BUCKETS);
	return table;
}

server_socket_t* ICACHE_FLASH_ATTR server_socket_create(server_socket_t* server, uint16_t port) {
	DEBUG_FUNCTION_START();
	if (server == NULL) server = (server_socket_t*) m_malloc(sizeof(server_socket_t));

	espconn_t* conn = (espconn_t*) m_malloc(sizeof(espconn_t));
	ets_memset(conn, 0, sizeof(espconn_t));
	conn->proto.tcp = (esp_tcp*) m_malloc(sizeof(esp_tcp));
	ets_memset(conn->proto.tcp, 0, sizeof(esp_tcp));

	server->conn = conn;
	server->reverse = NULL;
	server->connect_cb = NULL;

	conn->type = ESPCONN_TCP;
	conn->proto.tcp->local_port = port;
	conn->reverse = server;
	espconn_create(conn);
	espconn_regist_connectcb(conn, connect);

	int_table_put(server_table(), port, server);
	return server;
}

void ICACHE_FLASH_ATTR server_socket_destroy(server_socket_t* server, bool all) {
	DEBUG_FUNCTION_START();
	uint16 port = server_socket_port(server);
	if (int_table_remove(server_table(), port) == NULL) {
		return;
	}

	server_socket_close(server);
	espconn_delete(server->conn);

	os_free(server->conn->proto.tcp);
	os_free(server->conn);
	if (all) os_free(server);
}

static void connect(void* arg) {
	DEBUG_FUNCTION_START();

	espconn_t* conn = (espconn_t*) arg;

	uint16 port = conn->proto.tcp->local_port;
	server_socket_t* server = (server_socket_t*) int_table_get(server_table(), port);
	if (server == NULL) {
		DEBUG("server: not found on port %d", port);
		espconn_disconnect(conn);
		return;
	}

	if (server->connect_cb == NULL) {
		DEBUG("server: connect callback null - exit");
		espconn_disconnect(conn);
		return;
	}

	socket_t* socket = socket_create(NULL, conn);
	socket->server = server;

	server->connect_cb(server, socket);
}

socket_t* ICACHE_FLASH_ATTR socket_create(socket_t* socket, espconn_t* conn) {
	DEBUG_FUNCTION_START();
	if (socket == NULL) socket = (socket_t*) os_zalloc(sizeof(socket_t));

	socket->conn = conn;
	socket->conn_destroy = false;
	socket->server = NULL;
	socket->reverse = NULL;
	socket->receive_cb = NULL;
	socket->sent_cb = NULL;
	socket->error_cb = NULL;
	socket->disconnect_cb = NULL;

	conn->reverse = socket;
	espconn_regist_recvcb(conn, receive);
	espconn_regist_sentcb(conn, sent);
	espconn_regist_reconcb(conn, error);
	espconn_regist_disconcb(conn, disconnect);

	return socket;
}

socket_t* ICACHE_FLASH_ATTR socket_create_tcp(socket_t* socket, void* ip, uint16 port) {
	espconn_t* conn = (espconn_t*) os_zalloc(sizeof(espconn_t));
	ets_memset(conn, 0, sizeof(espconn_t));
	conn->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
	ets_memset(conn->proto.tcp, 0, sizeof(esp_tcp));

	conn->type = ESPCONN_TCP;
	conn->proto.tcp->local_port = espconn_port();
	memcpy(conn->proto.tcp->remote_ip, ip, 4);
	conn->proto.tcp->remote_port = port;
	espconn_create(conn);

	socket = socket_create(socket, conn);
	socket->conn_destroy = true;
	return socket;
}

socket_t* ICACHE_FLASH_ATTR socket_create_udp(socket_t* socket, void* ip, uint16 port) {
	espconn_t* conn = (espconn_t*) os_zalloc(sizeof(espconn_t));
	ets_memset(conn, 0, sizeof(espconn_t));
	conn->proto.udp = (esp_udp*) os_zalloc(sizeof(esp_udp));
	ets_memset(conn->proto.udp, 0, sizeof(esp_udp));

	conn->type = ESPCONN_UDP;
	conn->proto.udp->local_port = espconn_port();
	memcpy(conn->proto.udp->remote_ip, ip, 4);
	conn->proto.udp->remote_port = port;
	espconn_create(conn);

	socket = socket_create(socket, conn);
	socket->conn_destroy = true;
	return socket;
}

void ICACHE_FLASH_ATTR socket_destroy(socket_t* socket, bool all) {
	DEBUG_FUNCTION_START();

	socket_close(socket);

	if (socket->conn_destroy) {
		espconn_delete(socket->conn);
		os_free(socket->conn->proto.tcp);
		os_free(socket->conn);
	}

	if (all) os_free(socket);
}

static void receive(void* arg, char* data, uint16_t length) {
	espconn_t* conn = (espconn_t*) arg;

	socket_t* socket = (socket_t*) conn->reverse;
	if (socket->receive_cb != NULL) socket->receive_cb(socket, data, length);
	else DEBUG_FUNCTION("uncatched receive");
}

static void sent(void* arg) {
	espconn_t* conn = (espconn_t*) arg;

	socket_t* socket = (socket_t*) conn->reverse;
	if (socket->sent_cb != NULL) socket->sent_cb(socket);
	else DEBUG_FUNCTION("uncatched sent");
}

static void error(void* arg, int8_t error) {
	espconn_t* conn = (espconn_t*) arg;

	socket_t* socket = (socket_t*) conn->reverse;
	if (socket->error_cb != NULL) socket->error_cb(socket, error);
	else DEBUG_FUNCTION("uncatched error");
}

static void disconnect(void* arg) {
	espconn_t* conn = (espconn_t*) arg;

	socket_t* socket = (socket_t*) conn->reverse;
	if (socket->disconnect_cb != NULL) socket->disconnect_cb(socket);
	else DEBUG_FUNCTION("uncatched disconnect");
}
