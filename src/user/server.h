
#ifndef SERVER_H_
#define SERVER_H_


#include "c_types.h"
#include "os_type.h"

#include "util.h"
#include "signal.h"
#include "network/socket.h"
#include "network/beacon.h"


#define SERVER_VERSION 1

#define IR_DEFAULT_PORT 1234

#define IR_DELAY_SOON 20

#define IR_TIME_LENGTH 2
#define IR_TIMES_MAX 128

#define IR_SWAP_ENDIAN true

#define IR_BUFFER_LENGTH (IR_TIMES_MAX * IR_TIME_LENGTH)
#define IR_SEND_BUFFER_LENGTH 1024

#define IR_GPIO_RECEIVE 2
#define IR_GPIO_SEND 0
#define IR_TIMEOUT_SIGNAL 100000
#define IR_TIMEOUT_PULSE 10000

#define IR_NAME_LENGTH_MAX 32

#define IR_BEACON_PORT 8888
#define IR_BEACON_INTERVAL 1000
#define IR_BEACON_HEAD_LENGTH 3
#define IR_BEACON_LENGTH_MAX (IR_BEACON_HEAD_LENGTH + IR_NAME_LENGTH_MAX)


typedef enum ir_packet_type ir_packet_type_t;
typedef enum ir_worker_state ir_worker_state_t;

typedef struct ir_server ir_server_t;
typedef struct ir_beacon ir_beacon_t;
typedef struct ir_worker ir_worker_t;

typedef void (*ir_config_cb_t) (ir_server_t* server, string_t* ssid, string_t* password);


enum ir_packet_type {
	IR_SEND_REQUEST,
	IR_SEND_RESPONSE,
	IR_RECEIVE_REQUEST,
	IR_RECEIVE_RESPONSE,
	IR_CONFIG_REQUEST,
	IR_CONFIG_RESPONSE
};

enum ir_worker_state {
	IR_WORKER_READY,
	IR_WORKER_REQUEST,
	IR_WORKER_PROCESS,
	IR_WORKER_RESPONSE,
	IR_WORKER_FINISH
};

struct ir_worker {
	socket_t* socket;
	ir_server_t* server;

	stream_t in;
	stream_t out;
	stack_buffer_t buffer;

	ir_worker_state_t state;
	bool send_lock;
	os_timer_t timer;

	struct {
		uint8_t type;
		uint16_t length;

		uint8_t state;
		uint16_t read;

		union {
			struct {
				uint8_t state;

				uint32_t frequency;
				uint16_t length;

				uint16_t read;
			} send;
			struct {
			} receive;
			struct {
				uint8_t state;

				string_t ssid;
				string_t password;

				uint8_t length;
			} config;
		};
	} request;

	struct {
		uint8_t state;

		union {
			struct {
			} send;
			struct {
				uint8_t state;
			} receive;
			struct {
			} config;
		};
	} process;

	struct {
		uint8_t state;

		union {
			struct {
			} send;
			struct {
				uint8_t state;
				uint16_t written;
			} receive;
			struct {
			} config;
		};
	} response;
};

struct ir_server {
	server_socket_t socket;

	beacon_t beacon;
	stream_t beacon_out;

	ir_worker_t worker;
	signal_station_t station;

	bool running;
	stack_buffer_t name;

	ir_config_cb_t config_cb;
};


ir_server_t* ir_server_create(ir_server_t* server, uint16_t port);
void ir_server_destroy(ir_server_t* server, bool all);
void ir_server_start(ir_server_t* server);
void ir_server_stop(ir_server_t* server);

ir_worker_t* ir_worker_create(ir_worker_t* worker, ir_server_t* server, socket_t* socket);
void ir_worker_destroy(ir_worker_t* worker, bool all);
void ir_worker_reset(ir_worker_t* worker);


#endif /* SERVER_H_ */
