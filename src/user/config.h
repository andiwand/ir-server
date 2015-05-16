
#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_


#include "c_types.h"
#include "user_config.h"

#include "util.h"
#include "server.h"
#include "network.h"


#define CONFIG_MAGIC 0x414E4449
#define CONFIG_VERSION 1


typedef struct config config_t;


struct config {
	uint32_t address;
	uint16_t position;

	struct {
		uint32_t magic;
		uint8_t version;
		string_t name;
		struct {
			string_t ssid;
			string_t password;
		} wifi;
		uint16_t data_length;
	} head;

	network_t* network;
	ir_server_t* server;
};


bool config_load(config_t* config);
bool config_save(config_t* config);


#endif /* USER_CONFIG_H_ */
