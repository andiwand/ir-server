
#include "server.h"

#include "user_interface.h"
#include "osapi.h"
#include "ip_addr.h"

#include "memory.h"
#include "util.h"
#include "network/socket.h"
#include "network/beacon.h"

#include "debug/debug_on.h"


static void beacon_pack(ir_server_t* server);

static void worker_stop(ir_worker_t* worker);
static void worker_run(ir_worker_t* worker);
static void worker_run_soon(ir_worker_t* worker);
static bool read_request(ir_worker_t* worker);
static bool read_send_request(ir_worker_t* worker);
static bool read_receive_request(ir_worker_t* worker);
static bool read_config_request(ir_worker_t* worker);
static bool process(ir_worker_t* worker);
static bool process_send(ir_worker_t* worker);
static bool process_receive(ir_worker_t* worker);
static void send_buffer(ir_worker_t* worker);
static bool write_response(ir_worker_t* worker);
static bool write_response_head(ir_worker_t* worker, uint8_t type, uint16_t length);
static bool write_send_response(ir_worker_t* worker);
static bool write_receive_response(ir_worker_t* worker);
static bool write_config_response(ir_worker_t* worker);
static bool finish(ir_worker_t* worker);
static bool finish_config(ir_worker_t* worker);

static void connect(server_socket_t* server, socket_t* client);
static void receive(socket_t* client, uint8_t* data, uint16_t length);
static void sent(socket_t* client);
static void disconnect(socket_t* client);

static void signal_received(signal_station_t* station);


ir_server_t* ICACHE_FLASH_ATTR ir_server_create(ir_server_t* server, uint16_t port) {
	if (server == NULL) server = (ir_server_t*) m_malloc(sizeof(ir_server_t));

	server_socket_create(&server->socket, port);
	server->socket.reverse = server;
	server->socket.connect_cb = connect;

	beacon_create(&server->beacon, IR_BEACON_PORT);
	server->beacon.interval = IR_BEACON_INTERVAL;
	stream_create(&server->beacon_out);
	server->beacon_out.swap_endian = IR_SWAP_ENDIAN;
	stack_buffer_create(&server->beacon_out.buffer, NULL, IR_BEACON_LENGTH_MAX);

	ir_worker_create(&server->worker, server, NULL);

	signal_station_create(&server->station);
	server->station.time_length = IR_TIME_LENGTH;
	server->station.signal_timeout = IR_TIMEOUT_SIGNAL;
	server->station.pulse_timeout = IR_TIMEOUT_PULSE;
	server->station.received_cb = signal_received;

	stack_buffer_create(&server->name, NULL, IR_NAME_LENGTH_MAX);

	server->running = false;

	return server;
}

void ICACHE_FLASH_ATTR ir_server_destroy(ir_server_t* server, bool all) {
	ir_server_stop(server);

	if (all) m_free(server);
}

void ICACHE_FLASH_ATTR ir_server_start(ir_server_t* server) {
	DEBUG_FUNCTION_START();

	if (server->running) return;

	beacon_pack(server);
	server_socket_accept(&server->socket);
	beacon_start(&server->beacon);

	server->running = true;
}

void ICACHE_FLASH_ATTR ir_server_stop(ir_server_t* server) {
	if (!server->running) return;

	server_socket_close(&server->socket); // TODO: check
	beacon_stop(&server->beacon);

	server->running = false;
}

static void beacon_pack(ir_server_t* server) {
	uint8_t name_length = stack_buffer_size(&server->name);
	uint16_t length = IR_BEACON_HEAD_LENGTH + name_length;
	if (length > server->beacon_out.buffer.length) {
		DEBUG_FUNCTION("buffer too small");
		server->beacon.data.start = 0;
		server->beacon.data.length = 0;
		return;
	}

	bool done = true;
	stream_t* s = &server->beacon_out;
	uint16_t port = server_socket_port(&server->socket);

	stream_reset(s);
	done &= stream_write_primitive(s, &port, 2);
	done &= stream_write_primitive(s, &name_length, 1);
	done &= stream_write(s, server->name.start, name_length);

	if (!done) {
		DEBUG_FUNCTION("write error");
		server->beacon.data.start = 0;
		server->beacon.data.length = 0;
		return;
	}

	server->beacon.data.start = server->beacon_out.buffer.start;
	server->beacon.data.length = length;
}

ir_worker_t* ICACHE_FLASH_ATTR ir_worker_create(ir_worker_t* worker, ir_server_t* server, socket_t* socket) {
	if (worker == NULL) worker = (ir_worker_t*) m_malloc(sizeof(ir_worker_t));

	stream_create(&worker->in);
	worker->in.swap_endian = IR_SWAP_ENDIAN;

	stream_create(&worker->out);
	worker->out.swap_endian = IR_SWAP_ENDIAN;
	stack_buffer_create(&worker->out.buffer, NULL, IR_SEND_BUFFER_LENGTH);

	stack_buffer_create(&worker->buffer, NULL, IR_BUFFER_LENGTH);

	ir_worker_reset(worker);
	worker->socket = socket;
	worker->server = server;

	return worker;
}

void ICACHE_FLASH_ATTR ir_worker_destroy(ir_worker_t* worker, bool all) {
	if (all) m_free(worker);
}

void ICACHE_FLASH_ATTR ir_worker_reset(ir_worker_t* worker) {
	if (worker->socket) socket_close(worker->socket);
	worker->socket = NULL;
	worker->state = IR_WORKER_READY;

	stream_reset(&worker->in);
	stream_reset(&worker->out);
	stack_buffer_reset(&worker->buffer);

	signal_station_reset(&worker->server->station);

	m_memset(&worker->request, 0, sizeof(worker->request));
	m_memset(&worker->process, 0, sizeof(worker->process));
	m_memset(&worker->response, 0, sizeof(worker->response));
}

static void ICACHE_FLASH_ATTR worker_stop(ir_worker_t* worker) {
	worker->state = IR_WORKER_FINISH;
	worker_run_soon(worker);
}

// TODO: worker timeout
// TODO: stop beacon
static void ICACHE_FLASH_ATTR worker_run(ir_worker_t* worker) {
	switch (worker->state) {
	case IR_WORKER_READY:
		worker->state = IR_WORKER_REQUEST;
		/* no break */
	case IR_WORKER_REQUEST:
		if (!read_request(worker)) break;
		worker->state = IR_WORKER_PROCESS;
		/* no break */
	case IR_WORKER_PROCESS:
		if (!process(worker)) break;
		worker->state = IR_WORKER_RESPONSE;
		/* no break */
	case IR_WORKER_RESPONSE:
		if (!write_response(worker)) break;
		worker->state = IR_WORKER_FINISH;
		/* no break */
	case IR_WORKER_FINISH:
	{
		if (!finish(worker)) break;
		ir_worker_reset(worker);
		break;
	}
	default:
		DEBUG_FUNCTION("illegal state");
		worker_stop(worker);
		break;
	}
}

static void ICACHE_FLASH_ATTR worker_run_soon(ir_worker_t* worker) {
	os_timer_disarm(&worker->timer);
	os_timer_setfn(&worker->timer, worker_run, worker);
	os_timer_arm(&worker->timer, IR_DELAY_SOON, false);
}

static bool ICACHE_FLASH_ATTR read_request(ir_worker_t* worker) {
	switch (worker->request.state) {
	case 0:
		if (!stream_read_primitive(&worker->in, &worker->request.type, 1)) break;
		DEBUG("type %d", worker->request.type);
		worker->request.state++;
		/* no break */
	case 1:
		if (!stream_read_primitive(&worker->in, &worker->request.length, 2)) break;
		DEBUG("length %d", worker->request.length);
		worker->request.state++;
		/* no break */
	case 2:
	{
		bool done = false;
		uint8_t* start = worker->in.buffer.position;

		switch (worker->request.type) {
		case IR_SEND_REQUEST:
			done = read_send_request(worker);
			break;
		case IR_RECEIVE_REQUEST:
			done = read_receive_request(worker);
			break;
		case IR_CONFIG_REQUEST:
			done = read_config_request(worker);
			break;
		default:
			DEBUG_FUNCTION("illegal request");
			worker_stop(worker);
			return false;
		}

		uint16_t read = worker->in.buffer.position - start;
		worker->request.read += read;
		if (done ^ (worker->request.read == worker->request.length)) {
			DEBUG_FUNCTION("illegal read state");
			DEBUG("read %d/%d", worker->request.read, worker->request.length);
			// TODO: error
			worker_stop(worker);
			return false;
		}
		if (done && (worker->request.read == worker->request.length)) return true;
		break;
	}
	}

	return false;
}

static bool ICACHE_FLASH_ATTR read_send_request(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->request.send.state) {
	case 0:
		if (!stream_read_primitive(&worker->in, &worker->request.send.frequency, 4)) break;
		DEBUG("frequency %d", worker->request.send.frequency);
		worker->request.send.state++;
		/* no break */
	case 1:
		if (!stream_read_primitive(&worker->in, &worker->request.send.length, 2)) break;
		DEBUG("length %d", worker->request.send.length);
		if (worker->request.send.length > worker->buffer.length) {
			DEBUG_FUNCTION("buffer overflow");
			// TODO: error
			worker_stop(worker);
			return false;
		}
		worker->request.send.state++;
		/* no break */
	case 2:
		while (true) {
			if (worker->request.send.read == worker->request.send.length) {
				return true;
			}
			if (!stream_read_primitive(&worker->in, worker->buffer.position, IR_TIME_LENGTH)) break;
			worker->buffer.position += IR_TIME_LENGTH;
			worker->request.send.read++;
		}
	}

	return false;
}

static bool ICACHE_FLASH_ATTR read_receive_request(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();
	return true;
}

static bool ICACHE_FLASH_ATTR read_config_request(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->request.config.state) {
	case 0:
		if (!stream_read_primitive(&worker->in, &worker->request.config.length, 1)) break;
		DEBUG("name length %d", worker->request.config.length);
		if (worker->request.config.length > worker->server->name.length) {
			DEBUG_FUNCTION("buffer overflow");
			worker_stop(worker);
			return false;
		}
		stack_buffer_reset(&worker->server->name);
		worker->request.config.state++;
		/* no break */
	case 1:
		if (!stream_read(&worker->in, worker->server->name.start, worker->request.config.length)) break;
		//stack_buffer_skip(&worker->buffer, worker->request.config.length);
		stack_buffer_skip(&worker->server->name, worker->request.config.length);
		worker->request.config.state++;
		/* no break */
	case 2:
		if (!stream_read_primitive(&worker->in, &worker->request.config.length, 1)) break;
		DEBUG("ssid length %d", worker->request.config.length);
		if (worker->request.config.length > stack_buffer_left(&worker->buffer)) {
			DEBUG_FUNCTION("buffer overflow");
			worker_stop(worker);
			return false;
		}
		string_create(&worker->request.config.ssid, worker->buffer.position, worker->request.config.length, false);
		worker->request.config.state++;
		/* no break */
	case 3:
		if (!stream_read(&worker->in, worker->buffer.position, worker->request.config.ssid.length)) break;
		stack_buffer_skip(&worker->buffer, worker->request.config.ssid.length);
		worker->request.config.state++;
		/* no break */
	case 4:
		if (!stream_read_primitive(&worker->in, &worker->request.config.length, 1)) break;
		DEBUG("ssid length %d", worker->request.config.length);
		if (worker->request.config.length > stack_buffer_left(&worker->buffer)) {
			DEBUG_FUNCTION("buffer overflow");
			worker_stop(worker);
			return false;
		}
		string_create(&worker->request.config.password, worker->buffer.position, worker->request.config.length, false);
		worker->request.config.state++;
		/* no break */
	case 5:
		if (!stream_read(&worker->in, worker->buffer.position, worker->request.config.password.length)) break;
		stack_buffer_skip(&worker->buffer, worker->request.config.ssid.length);
		return true;
	}

	return false;
}

static bool ICACHE_FLASH_ATTR process(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->process.state) {
	case 0:
	{
		worker_run_soon(worker);
		worker->process.state++;
		break;
	}
	case 1:
	{
		switch (worker->request.type) {
		case IR_SEND_REQUEST:
			return process_send(worker);
		case IR_RECEIVE_REQUEST:
			return process_receive(worker);
		case IR_CONFIG_REQUEST:
			return true;
		default:
			DEBUG_FUNCTION("illegal state");
			worker_stop(worker);
			return false;
		}

		return false;
	}
	}

	return false;
}

static bool ICACHE_FLASH_ATTR process_send(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	signal_station_t* station = (signal_station_t*) &worker->server->station;
	station->gpio = IR_GPIO_SEND;
	station->frequency = worker->request.send.frequency;
	station->times = worker->buffer; // TODO: remove mem copy

	signal_send(station);
	return true;
}

static bool ICACHE_FLASH_ATTR process_receive(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->process.receive.state) {
	case 0:
	{
		signal_station_t* station = (signal_station_t*) &worker->server->station;
		station->gpio = IR_GPIO_RECEIVE;
		station->times = worker->buffer; // TODO: remove mem copy
		station->reverse = worker;

		signal_receive_next(&worker->server->station);
		worker->process.receive.state++;
		break;
	}
	case 1:
		DEBUG("times %d", signal_station_size(&worker->server->station));
		return true;
	}

	return false;
}

static void ICACHE_FLASH_ATTR send_buffer(ir_worker_t* worker) {
	if (worker->send_lock) {
		DEBUG_FUNCTION("locked");
		return;
	}

	stream_t* s = &worker->out;
	socket_send(worker->socket, s->buffer.start, stack_buffer_size(&s->buffer));
	worker->send_lock = false;
	stream_reset(s);
}

static bool ICACHE_FLASH_ATTR write_response(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->response.state) {
	case 0:
	{
		bool done = false;
		switch (worker->request.type) {
		case IR_SEND_REQUEST:
			done = write_send_response(worker);
			break;
		case IR_RECEIVE_REQUEST:
			done = write_receive_response(worker);
			break;
		case IR_CONFIG_REQUEST:
			done = write_config_response(worker);
			break;
		default:
			DEBUG_FUNCTION("unimplemented response");
			worker_stop(worker);
			return false;
		}
		if (!done) break;
		worker->response.state++;
	}
		/* no break */
	case 1:
		return !worker->send_lock;
	}
}

static bool ICACHE_FLASH_ATTR write_response_head(ir_worker_t* worker, uint8_t type, uint16_t length) {
	bool done = true;
	stream_t* s = &worker->out;
	done &= stream_write_primitive(s, &type, 1);
	done &= stream_write_primitive(s, &length, 2);
	if (!done) {
		DEBUG_FUNCTION("write error");
		worker_stop(worker);
	}
	return done;
}

static bool ICACHE_FLASH_ATTR write_send_response(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	if (!write_response_head(worker, IR_SEND_RESPONSE, 0)) return false;
	send_buffer(worker);
	return true;
}

static bool ICACHE_FLASH_ATTR write_receive_response(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	switch (worker->response.receive.state) {
	case 0:
	{
		uint16_t length = 4 + 2 + IR_TIME_LENGTH * signal_station_size(&worker->server->station);
		if (!write_response_head(worker, IR_RECEIVE_RESPONSE, length)) return false;
		worker->response.receive.state++;
	}
		/* no break */
	case 1:
	{
		bool done = true;
		stream_t* s = &worker->out;
		uint32_t frequency = 0; // TODO: field
		done &= stream_write_primitive(s, &frequency, 4);
		uint16_t length = signal_station_size(&worker->server->station);
		done &= stream_write_primitive(s, &length, 2);
		if (!done) {
			DEBUG_FUNCTION("buffer too small");
			worker_stop(worker);
			return false;
		}
		worker->response.receive.state++;
	}
		/* no break */
	case 2:
	{
		stream_t* s = &worker->out;
		uint8_t* times = (uint8_t*) (worker->buffer.start + IR_TIME_LENGTH * worker->response.receive.written);
		bool done = false;
		while (true) {
			if (worker->response.receive.written >= signal_station_size(&worker->server->station)) {
				done = true;
				break;
			}
			if (!stream_write_primitive(s, times, IR_TIME_LENGTH)) {
				done = false;
				break;
			}
			times += IR_TIME_LENGTH;
			worker->response.receive.written++;
		}
		send_buffer(worker);
		return done;
	}
	}

	return false;
}

static bool ICACHE_FLASH_ATTR write_config_response(ir_worker_t* worker) {
	DEBUG_FUNCTION_START();

	if (!write_response_head(worker, IR_CONFIG_RESPONSE, 0)) return false;
	send_buffer(worker);
	return true;
}

static bool ICACHE_FLASH_ATTR finish(ir_worker_t* worker) {
	switch (worker->request.type) {
	case IR_SEND_REQUEST:
		return true;
	case IR_RECEIVE_REQUEST:
		return true;
	case IR_CONFIG_REQUEST:
		return finish_config(worker);
	default:
		DEBUG_FUNCTION("illegal request");
		worker_stop(worker);
		return false;
	}

	return false;
}

static bool ICACHE_FLASH_ATTR finish_config(ir_worker_t* worker) {
	ir_server_t* server = worker->server;

	beacon_pack(server);
	if (server->config_cb != NULL) {
		server->config_cb(server, &worker->request.config.ssid, &worker->request.config.password);
	}

	return true;
}

static void connect(server_socket_t* server, socket_t* client) {
	ir_server_t* ir_server = (ir_server_t*) server->reverse;
	if (ir_server->worker.state != IR_WORKER_READY) {
		socket_close(client);
		return;
	}

	ir_server->worker.socket = client;

	client->reverse = &ir_server->worker;
	client->receive_cb = receive;
	client->sent_cb = sent;
	client->disconnect_cb = disconnect;
}

static void receive(socket_t* client, uint8_t* data, uint16_t length) {
	DEBUG_FUNCTION_START();
	DEBUG("tcp length %d", length);

	ir_worker_t* worker = (ir_worker_t*) client->reverse;
	stream_data(&worker->in, data, length);
	worker_run(worker);
}

static void sent(socket_t* client) {
	ir_worker_t* worker = (ir_worker_t*) client->reverse;
	worker->send_lock = true;
	worker_run(worker);
}

static void disconnect(socket_t* client) {
	ir_worker_t* worker = (ir_worker_t*) client->reverse;
	worker_stop(worker);
}

static void signal_received(signal_station_t* station) {
	DEBUG_FUNCTION_START();

	ir_worker_t* worker = (ir_worker_t*) station->reverse;
	worker_run(worker);
}
