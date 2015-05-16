#include "network.h"

#include "c_types.h"
#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"

#include "util.h"
#include "memory.h"

#include "debug/debug_on.h"


static bool is_station_connected();

static void network_configure_timer(network_t* network);
static void network_check(network_t* network);


network_t* ICACHE_FLASH_ATTR network_get() {
	static network_t network;
	return &network;
}

static ICACHE_FLASH_ATTR void network_configure_timer(network_t* network) {
	os_timer_disarm(&network->check_timer);
	if (network->check & (network_mode_get(network) == STATION_MODE)) {
		os_timer_setfn(&network->check_timer, network_check, network);
		os_timer_arm(&network->check_timer, network->check_interval, true);
	}
}

void ICACHE_FLASH_ATTR network_configure(network_t* network, bool autoconnect,
		bool check, uint16_t check_interval, bool failover,
		uint16_t failover_time) {
	network->autoconnect = autoconnect;
	network->check = check;
	network->check_interval = check_interval;
	network->failover = failover;
	network->failover_time = failover_time;

	network_configure_timer(network);
}

bool ICACHE_FLASH_ATTR network_configure_station(network_t* network,
		string_t* ssid, string_t* password, uint8_t* bssid) {
	station_config_t* config = &network->station_config;

	if (ssid != NULL) {
		if (ssid->length > sizeof(config->ssid)) {
			// TODO: error
			return false;
		}

		m_memcpy(config->ssid, ssid->buffer, ssid->length);
		m_memset(config->ssid + ssid->length, 0,
				sizeof(config->ssid) - ssid->length);
	}

	if (password != NULL) {
		if (password->length > sizeof(config->password)) {
			// TODO: error
			return false;
		}

		m_memcpy(config->password, password->buffer, password->length);
		m_memset(config->password + password->length, 0,
				sizeof(config->password) - password->length);
	}

	if (bssid == NULL) {
		config->bssid_set = false;
	} else {
		config->bssid_set = true;
		m_memcpy(config->bssid, bssid, sizeof(config->bssid));
	}

	return true;
}

bool ICACHE_FLASH_ATTR network_configure_softap(network_t* network,
		string_t* ssid, string_t* password, uint8_t channel, AUTH_MODE authmode,
		bool hidden, uint8_t max_connection, uint16_t beacon_interval) {
	softap_config_t* config = &network->softap_config;

	if (ssid->length > sizeof(config->ssid)) {
		// TODO: error
		return false;
	}
	m_memcpy(config->ssid, ssid->buffer, ssid->length);
	m_memset(config->ssid + ssid->length, 0,
			sizeof(config->ssid) - ssid->length);
	config->ssid_len = ssid->length;
	config->ssid_hidden = hidden;

	if (password != NULL) {
		if (password->length > sizeof(config->password)) {
			// TODO: error
			return false;
		}
		m_memcpy(config->password, password->buffer, password->length);
		m_memset(config->password + password->length, 0,
				sizeof(config->password) - password->length);
	}

	config->channel = channel;
	config->authmode = authmode;
	config->max_connection = max_connection;
	config->beacon_interval = beacon_interval;

	return true;
}

void ICACHE_FLASH_ATTR network_station_connect(network_t* network) {
	wifi_station_connect();
}

void ICACHE_FLASH_ATTR network_station_disconnect(network_t* network) {
	wifi_station_disconnect();
}

bool ICACHE_FLASH_ATTR network_mode_set(network_t* network, uint8_t mode) {
	bool result = wifi_set_opmode(mode);
	if (!result)
		return false;

	switch (mode) {
	case NULL_MODE:
		break;
	case STATION_MODE:
		result &= wifi_station_set_auto_connect(network->autoconnect);
		result &= wifi_station_set_config(&network->station_config);
		break;
	case SOFTAP_MODE:
		result &= wifi_softap_set_config(&network->softap_config);
		break;
	case STATIONAP_MODE:
		result &= wifi_station_set_auto_connect(network->autoconnect);
		result &= wifi_station_set_config(&network->station_config);
		result &= wifi_softap_set_config(&network->softap_config);
		break;
	default:
		// TODO: error
		result = false;
		break;
	}

	network_configure_timer(network);

	return result;
}

uint8_t ICACHE_FLASH_ATTR network_mode_get(network_t* network) {
	return wifi_get_opmode();
}

static void ICACHE_FLASH_ATTR network_check(network_t* network) {
	if (network_mode_get(network) == SOFTAP_MODE) {
		DEBUG_FUNCTION("illegal state");
		return;
	}

	bool connected = is_station_connected();

	switch (network->state) {
	case NETWORK_FAILOVER:
	case NETWORK_CONNECTING:
		if (connected) {
			if (network->state == NETWORK_FAILOVER) {
				network_mode_set(network, STATION_MODE);
			}

			network->state = NETWORK_CONNECTED;
		} else {
			if (wifi_station_dhcpc_status() == DHCP_STOPPED) {
				wifi_station_dhcpc_start();
			}

			if (network->state != NETWORK_FAILOVER) {
				if (network->failover_start == 0) {
					network->failover_start = system_get_time();
				}

				uint16 time = (system_get_time() - network->failover_start)
						/ 1000;
				if (time >= network->failover_time) {
					network->failover_start = 0;
					network_mode_set(network, STATIONAP_MODE);
				}
			}
		}
		break;
	case NETWORK_CONNECTED:
		if (!connected) {
			network->state = NETWORK_CONNECTING;
		}
		break;
	}
}

static bool ICACHE_FLASH_ATTR is_station_connected() {
	ip_info_t info;
	wifi_get_ip_info(STATION_IF, &info);
	return info.ip.addr | info.netmask.addr | info.gw.addr;
}
