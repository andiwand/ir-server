
#include "config.h"

#include "c_types.h"
#include "spi_flash.h"

#include "util.h"
#include "memory.h"


static bool data_read(config_t* config, void* address, void* start, uint16_t length);
static bool data_write(config_t* config, void* address, void* start, uint16_t length);
static void* data_write_auto(config_t* config, void* start, uint16_t length);


static ICACHE_FLASH_ATTR bool data_read(config_t* config, void* address, void* start, uint16_t length) {
	if (address == NULL) return false;

	uint32_t flash_address = config->address + sizeof(config->head) + ((uint32_t) address) - 1;
	SpiFlashOpResult result = spi_flash_read(flash_address, start, length);

	return result == SPI_FLASH_RESULT_OK;
}

static ICACHE_FLASH_ATTR bool data_write(config_t* config, void* address, void* start, uint16_t length) {
	if (start == NULL) return false;
	if (address == NULL) return false;

	uint32_t flash_address = config->address + sizeof(config->head) + ((uint32_t) address) - 1;
	SpiFlashOpResult result = spi_flash_write(flash_address, start, length);

	return result == SPI_FLASH_RESULT_OK;
}

static ICACHE_FLASH_ATTR void* data_write_auto(config_t* config, void* start, uint16_t length) {
	if (start == NULL) return NULL;

	void* address = (void*) (1 + config->position);
	// skip (pretty sure) unwriteable bytes
	uint32_t offset = config->address + sizeof(config->head) + config->position;
	if ((offset & 3) != 0) {
		address = (void*) (((uint32_t) address) + 4 - (offset & 3));
	}
	data_write(config, address, start, length);
	config->position += length;

	return address;
}

bool ICACHE_FLASH_ATTR config_load(config_t* config) {
	DEBUG_FUNCTION_START();

	// read head
	SpiFlashOpResult result = spi_flash_read(config->address, (uint32*) &config->head, sizeof(config->head));
	if (result != SPI_FLASH_RESULT_OK) return false;

	// check head
	if (config->head.magic != CONFIG_MAGIC) return false;
	if (config->head.version != CONFIG_VERSION) return false;
	if (config->head.name.length > stack_buffer_left(&config->server->name)) return false;
	if (config->head.wifi.ssid.length > sizeof(config->network->station_config.ssid)) return false;
	if (config->head.wifi.password.length > sizeof(config->network->station_config.password)) return false;

	// read data
	data_read(config, config->head.name.buffer, config->server->name.start, config->head.name.length);
	stack_buffer_skip(&config->server->name, config->head.name.length);

	data_read(config, config->head.wifi.ssid.buffer, config->network->station_config.ssid,
			config->head.wifi.ssid.length);
	m_memset(config->network->station_config.ssid + config->head.wifi.ssid.length, 0,
			sizeof(config->network->station_config.ssid) - config->head.wifi.ssid.length);

	data_read(config, config->head.wifi.password.buffer, config->network->station_config.password,
			config->head.wifi.password.length);
	m_memset(config->network->station_config.password + config->head.wifi.password.length, 0,
			sizeof(config->network->station_config.password) - config->head.wifi.password.length);

	return true;
}

bool ICACHE_FLASH_ATTR config_save(config_t* config) {
	SpiFlashOpResult result;
	uint16_t length;

	// clear data
	result = spi_flash_erase_sector(config->address >> 12);
	if (result != SPI_FLASH_RESULT_OK) {
		DEBUG_FUNCTION("erase failed");
		return false;
	}

	// write data
	config->head.name.buffer = data_write_auto(config, config->server->name.start, stack_buffer_size(&config->server->name));
	config->head.name.length = stack_buffer_size(&config->server->name);

	length = strlen((char*) config->network->station_config.ssid);
	length = MIN(length, sizeof(config->network->station_config.ssid));
	config->head.wifi.ssid.buffer = data_write_auto(config, config->network->station_config.ssid, length);
	config->head.wifi.ssid.length = length;

	length = strlen((char*) config->network->station_config.password);
	length = MIN(length, sizeof(config->network->station_config.password));
	config->head.wifi.password.buffer = data_write_auto(config, config->network->station_config.password, length);
	config->head.wifi.password.length = length;

	// write head
	config->head.magic = CONFIG_MAGIC;
	config->head.version = CONFIG_VERSION;
	config->head.data_length = config->position;

	result = spi_flash_write(config->address, (uint32*) &config->head, sizeof(config->head));
	if (result != SPI_FLASH_RESULT_OK) {
		DEBUG_FUNCTION("write head failed");
		return false;
	}

	return true;
}
