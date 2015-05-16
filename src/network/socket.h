
#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_


#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

#include "util.h"

#include "debug/debug_on.h"


#define SERVER_TABLE_BUCKETS 8


typedef struct espconn espconn_t;
typedef struct server_socket server_socket_t;
typedef struct socket socket_t;

typedef void (*server_connect_cb_t) (server_socket_t* server_socket, socket_t* client);
typedef void (*socket_receive_cb_t) (socket_t* client, uint8_t* data, uint16_t length);
typedef void (*socket_sent_cb_t) (socket_t* client);
typedef void (*socket_error_cb_t) (socket_t* client, int8_t error);
typedef void (*socket_disconnect_cb_t) (socket_t* client);


struct server_socket {
	// TODO: remove pointer
	espconn_t* conn;

	void* reverse;
	server_connect_cb_t connect_cb;
};

struct socket {
	// TODO: remove pointer
	espconn_t* conn;
	bool conn_destroy;
	server_socket_t* server;

	void* reverse;
	socket_receive_cb_t receive_cb;
	socket_sent_cb_t sent_cb;
	socket_error_cb_t error_cb;
	socket_disconnect_cb_t disconnect_cb;
};


server_socket_t* server_socket_create(server_socket_t* server, uint16_t port);
void server_socket_destroy(server_socket_t* server, bool all);
static inline void server_socket_accept(server_socket_t* server) {
	espconn_accept(server->conn);
}
static inline void server_socket_close(server_socket_t* server) {
	DEBUG_FUNCTION_START();
	if (server->conn->state != ESPCONN_CLOSE) espconn_disconnect(server->conn);
}
static inline uint16_t server_socket_port(server_socket_t* server) {
	return server->conn->proto.tcp->local_port;
}

socket_t* socket_create(socket_t* socket, espconn_t* conn);
socket_t* socket_create_tcp(socket_t* socket, void* ip, uint16_t port);
socket_t* socket_create_udp(socket_t* socket, void* ip, uint16_t port);
void socket_destroy(socket_t* socket, bool all);
static inline void socket_close(socket_t* socket) {
	DEBUG_FUNCTION_START();
	if (socket == NULL) return;
	if (socket->conn == NULL) return;
	if (socket->conn->state == ESPCONN_CLOSE) return;
	espconn_disconnect(socket->conn);
}
static inline void socket_send(socket_t* socket, uint8_t* buffer, uint16_t length) {
	espconn_sent(socket->conn, buffer, length);
}


#endif /* INCLUDE_SOCKET_H_ */
