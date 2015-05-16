
#include "osapi.h"
#include "user_interface.h"
#include "gpio.h"
#include "c_types.h"

#include "util.h"
#include "config.h"
#include "network.h"
#include "server.h"

#include "debug/debug_on.h"


#define DEBUG_BIT_RATE 115200

#define UPDATE_PORT 4444

#define NETWORK_INTERVAL 2000
#define NETWORK_TIMEOUT 5000

#define GENERATE_NAME_PREFIX STRING("IR-")

#define CONFIG_ADDRESS 0x40000


static config_t config;


static bool generate_name(stack_buffer_t* buffer);

static void config_changed(ir_server_t* server, string_t* ssid, string_t* password);


static bool ICACHE_FLASH_ATTR generate_name(stack_buffer_t* buffer) {
	uint16_t length = GENERATE_NAME_PREFIX.length + 4 + 1;
	if (length > stack_buffer_left(buffer)) return false;

	char* tmp = buffer->position;
	strcpy(tmp, GENERATE_NAME_PREFIX.buffer);
	tmp += GENERATE_NAME_PREFIX.length;
	uint32 chip = system_get_chip_id();
	uint16 magic = ((chip >> 16) & 0xFFFF) ^ (chip & 0xFFFF);
	tmp += os_sprintf(tmp, "%04x", magic);
	stack_buffer_skip(buffer, length - 1);

	return true;
}

static void config_changed(ir_server_t* server, string_t* ssid, string_t* password) {
	DEBUG_FUNCTION_START();

	if (ssid->length != 0) {
		network_configure_station(config.network, ssid, password, NULL);
		network_station_disconnect(config.network);
		network_station_connect(config.network);
	}

	config_save(&config);
}

void user_init(void) {
	wifi_station_ap_number_set(0);

	DEBUG_INIT(DEBUG_BIT_RATE);
	DEBUG("");
	DEBUG("start init");

	DEBUG("free heap %d", system_get_free_heap_size());
	DEBUG("user%d.bin is running", system_upgrade_userbin_check() + 1);

	network_t* network = network_get();
	ir_server_t* server = ir_server_create(NULL, IR_DEFAULT_PORT);
	server->config_cb = config_changed;

	DEBUG("init config");
	config.address = CONFIG_ADDRESS;
	config.server = server;
	config.network = network;

	DEBUG("read config");
	if (!config_load(&config)) {
		DEBUG("config not able to load, create new");
		config_save(&config);
	} else {
		DEBUG_STRING(server->name.start, stack_buffer_size(&server->name));
		DEBUG("");
		DEBUG_STRING(network->station_config.ssid, 32);
		DEBUG("");
		DEBUG_STRING(network->station_config.password, 64);
		DEBUG("");
	}

	if (stack_buffer_size(&server->name) <= 0) {
		DEBUG_FUNCTION("autogenerate name");
		generate_name(&server->name);
	}

	DEBUG("init wifi");
	string_t softap_ssid = {server->name.start, stack_buffer_size(&server->name)};
	network_configure(network, true, true, NETWORK_INTERVAL, true, NETWORK_TIMEOUT);
	network_configure_station(network, NULL, NULL, NULL);
	network_configure_softap(network, &softap_ssid, NULL, 0, AUTH_OPEN, false, 4, 0);
	network_mode_set(network, STATION_MODE);

	ir_server_start(server);

	DEBUG("done init");
}
