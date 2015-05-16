
#ifndef USER_NETWORK_H_
#define USER_NETWORK_H_


#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"

#include "util.h"


typedef struct station_config station_config_t;
typedef struct softap_config softap_config_t;
typedef struct ip_info ip_info_t;

typedef struct network network_t;

typedef enum network_state network_state_t;


enum network_state {
	NETWORK_CONNECTING,
	NETWORK_CONNECTED,
	NETWORK_FAILOVER
};

struct network {
	bool autoconnect;
	bool check;
	uint16_t check_interval;
	os_timer_t check_timer;
	bool failover;
	uint16_t failover_time;

	station_config_t station_config;
	softap_config_t softap_config;

	network_state_t state;
	uint32_t failover_start;
};


network_t* ICACHE_FLASH_ATTR network_get();

void network_configure(network_t* network, bool autoconnect, bool check, uint16_t check_interval,
		bool failover, uint16_t failover_time);
bool network_configure_station(network_t* network, string_t* ssid, string_t* password, uint8_t* bssid);
bool network_configure_softap(network_t* network, string_t* ssid, string_t* password, uint8_t channel,
		AUTH_MODE authmode, bool hidden, uint8_t max_connection, uint16_t beacon_interval);
void network_station_connect(network_t* network);
void network_station_disconnect(network_t* network);
bool network_mode_set(network_t* network, uint8_t mode);
uint8_t network_mode_get(network_t* network);


#endif /* USER_NETWORK_H_ */
