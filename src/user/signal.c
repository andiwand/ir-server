
#include "signal.h"

#include "gpio.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"

#include "memory.h"
#include "util.h"
#include "debug/debug.h"


static inline bool gpio_read(signal_station_t* station);
static inline void gpio_write(signal_station_t* station, bool bit);

static void init(signal_station_t* station);
static void init_in(signal_station_t* station);
static void init_out(signal_station_t* station);

static void space(signal_station_t* station, uint32_t time);
static void mark(signal_station_t* station, uint32_t time);

static bool time_read(signal_station_t* station, uint32_t* time);
static bool time_write(signal_station_t* station, uint32_t* time);

static void gpio_callback(void* arg);


static inline bool gpio_read(signal_station_t* station) {
	//return READ_PERI_REG(station->gpio_addr);
	return GPIO_INPUT_GET(station->gpio_id);
}

static inline void gpio_write(signal_station_t* station, bool bit) {
	WRITE_PERI_REG(station->gpio_addr, bit);
}

static void ICACHE_FLASH_ATTR init(signal_station_t* station) {
	station->position = 0;
	station->gpio_id = GPIO_ID_PIN(station->gpio);
	station->gpio_addr = (void*) (PERIPHS_GPIO_BASEADDR + station->gpio_id);
	if (station->frequency > 0) {
		station->periodic_time_half = (1000000 / station->frequency) >> 1;
	} else {
		station->periodic_time_half = 0;
	}
}

static void ICACHE_FLASH_ATTR init_in(signal_station_t* station) {
	init(station);

	GPIO_DIS_OUTPUT(station->gpio_id);
}

static void ICACHE_FLASH_ATTR init_out(signal_station_t* station) {
	init(station);

	GPIO_OUTPUT_SET(station->gpio_id, 0);
}

static void space(signal_station_t* station, uint32_t time) {
	gpio_write(station, 0);
	os_delay_us(time);
}

static void mark(signal_station_t* station, uint32_t time) {
	uint16 n = time / station->periodic_time_half;
	uint16 i = 0;
	while (i < n) {
		gpio_write(station, ++i & 1);
		os_delay_us(station->periodic_time_half);
	}
	gpio_write(station, 0);
}

static bool ICACHE_FLASH_ATTR time_read(signal_station_t* station, uint32_t* time) {
	if (station->position >= signal_station_length(station)) return false;

	switch (station->time_length) {
	case 1:
		*time = ((uint8_t*) station->times.start)[station->position];
		break;
	case 2:
		*time = ((uint16_t*) station->times.start)[station->position];
		break;
	case 4:
		*time = ((uint32_t*) station->times.start)[station->position];
		break;
	default:
		DEBUG_FUNCTION("illegal time length");
		return false;
	}

	station->position++;
	return true;
}

static bool ICACHE_FLASH_ATTR time_write(signal_station_t* station, uint32_t* time) {
	if (station->position >= signal_station_length(station)) return false;

	switch (station->time_length) {
	case 1:
		*((uint8_t*) station->times.position) = *time;
		break;
	case 2:
		*((uint16_t*) station->times.position) = *time;
		break;
	case 4:
		*((uint32_t*) station->times.position) = *time;
		break;
	default:
		DEBUG_FUNCTION("illegal time length");
		return false;
	}

	station->times.position += station->time_length;
	station->position++;
	return true;
}

signal_station_t* ICACHE_FLASH_ATTR signal_station_create(signal_station_t* station) {
	if (station == NULL) station = (signal_station_t*) m_malloc(sizeof(signal_station_t));

	station->signal_timeout = DEFAULT_SIGNAL_TIMEOUT;
	station->pulse_timeout = DEFAULT_PULSE_TIMEOUT;

	return station;
}

void ICACHE_FLASH_ATTR signal_station_destry(signal_station_t* station, bool all) {
	if (all) m_free(station);
}

void ICACHE_FLASH_ATTR signal_station_reset(signal_station_t* station) {
	ETS_GPIO_INTR_DISABLE();
}

void ICACHE_FLASH_ATTR signal_send(signal_station_t* station) {
	DEBUG_FUNCTION_START();
	init_out(station);

	uint32_t time;
	uint8 i = 0;
	while (true) {
		if (!time_read(station, &time)) break;
		mark(station, time);
		if (!time_read(station, &time)) break;
		space(station, time);
	}
}

void ICACHE_FLASH_ATTR signal_receive(signal_station_t* station) {
	DEBUG_FUNCTION_START();
	init_in(station);

	bool read;
	bool current = gpio_read(station);
	uint32_t start = system_get_time();
	uint32_t last = start;
	uint32_t now;
	uint32_t time;
	uint16_t i = 0;

	while (true) {
		read = gpio_read(station);
		now = system_get_time();
		time = now - last;

		if ((now - start) >= station->signal_timeout) {
			DEBUG_FUNCTION("signal timeout");
			// TODO: error
			break;
		}
		if (time >= station->pulse_timeout) {
			DEBUG_FUNCTION("pulse timeout");
			break;
		}
		if (read == current) continue;
		if (i >= signal_station_length(station)) {
			DEBUG_FUNCTION("buffer overflow");
			// TODO: error
			break;
		}

		time_write(station, &time);
		last = now;
		current = read;
	}

	DEBUG_FUNCTION_END();
}

void ICACHE_FLASH_ATTR signal_receive_next(signal_station_t* station) {
	DEBUG_FUNCTION_START();
	init_in(station); // TODO: fix double init

	ETS_GPIO_INTR_DISABLE();
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(station->gpio));

	ETS_GPIO_INTR_ATTACH(gpio_callback, station); //(uint32_t) station->gpio
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_pin_intr_state_set(station->gpio_id, GPIO_PIN_INTR_ANYEGDE);
	ETS_GPIO_INTR_ENABLE();
}

static void gpio_callback(void* arg) {
	DEBUG_FUNCTION_START();
	ETS_GPIO_INTR_DISABLE();

	signal_station_t* station = (signal_station_t*) arg;
	signal_receive(station);
	station->received_cb(station);
}
