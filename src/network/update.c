
#include "network/update.h"

#include "c_types.h"
#include "mem.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "upgrade.h"

#include "debug/debug.h"


static struct espconn* update_socket;
static os_timer_t update_timer;
static update_state_t update_state;
static uint32 update_dest;
static uint32 update_pos;


void update_init(uint16 port) {
	update_socket = (struct espconn*) os_zalloc(sizeof(struct espconn));
	ets_memset(update_socket, 0, sizeof(struct espconn));
	update_socket->proto.tcp = (esp_tcp*) os_zalloc(sizeof(esp_tcp));
	ets_memset(update_socket->proto.tcp, 0, sizeof(esp_tcp));

	update_socket->type = ESPCONN_TCP;
	update_socket->proto.tcp->local_port = port;
	espconn_regist_connectcb(update_socket, update_listen);

	espconn_create(update_socket);
	espconn_accept(update_socket);

	update_state = UPDATE_READY;
	update_dest = (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) ? UPDATE_ADDR_USER2 : UPDATE_ADDR_USER1;
}

static void update_listen(void* arg) {
	struct espconn* client = arg;
	DEBUG("update: client connected");

	if (update_state != UPDATE_READY) {
		DEBUG("update: client kicked, busy");
		espconn_disconnect(client);
		return;
	}

	update_state = UPDATE_BUSY;
	update_pos = 0;

	system_upgrade_init();
	system_upgrade_flag_set(UPGRADE_FLAG_START);

	espconn_regist_recvcb(client, update_recv);
	espconn_regist_disconcb(client, update_discon);

	os_timer_disarm(&update_timer);
	os_timer_setfn(&update_timer, update_timeout);
	os_timer_arm(&update_timer, UPDATE_TIMEOUT, false);
}

static void update_discon(void* arg) {
	struct espconn* client = arg;
	DEBUG("update: disconnect");
	if (update_state != UPDATE_DONE) {
		update_state = UPDATE_READY;
		update_pos = 0;
		system_upgrade_deinit();
		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
	}
	if (client->state != ESPCONN_CLOSE) espconn_disconnect(client);
}

static void update_timeout(void* arg) {
	struct espconn* client = arg;
	if (update_state == UPDATE_DONE) return;
	DEBUG("update: client timeout");
	update_discon(client);
}

static void update_recv(void* arg, char* data, unsigned short len) {
	struct espconn* client = arg;

	if (update_state != UPDATE_BUSY) return;

	if (update_pos >= update_dest + UPDATE_SIZE) {
		DEBUG("update: done");
		update_state = UPDATE_DONE;
		system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
		update_discon(client);
		os_timer_disarm(&update_timer);
		os_timer_setfn(&update_timer, update_finish);
		os_timer_arm(&update_timer, UPDATE_FINISH_DELAY, false);
	} else if (update_pos + len > update_dest) {
		uint16 offset = 0;
		uint32 remaining = UPDATE_SIZE - (update_pos - update_dest);
		uint16 length = len;
		if (update_dest > update_pos) {
			offset = update_dest - update_pos;
			length -= offset;
		}
		if (remaining < length) {
			length = remaining;
		}
		system_upgrade(data + offset, length);
	}

	update_pos += len;
}

static void update_finish() {
	DEBUG("update: reboot");

	system_upgrade_reboot();
}
