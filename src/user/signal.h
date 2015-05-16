
#ifndef SIGNAL_H_
#define SIGNAL_H_


#include "c_types.h"

#include "util.h"


#define DEFAULT_SIGNAL_TIMEOUT 100000
#define DEFAULT_PULSE_TIMEOUT 10000


typedef struct signal_station signal_station_t;

typedef void (*signal_received_cb_t) (signal_station_t* station);


struct signal_station {
	uint8_t gpio;
	uint32_t frequency;
	stack_buffer_t times;
	uint8_t time_length;
	uint32_t signal_timeout;
	uint32_t pulse_timeout;

	uint8_t gpio_id;
	void* gpio_addr;
	uint16_t position;
	uint16_t periodic_time_half;

	void* reverse;
	signal_received_cb_t received_cb;
};


signal_station_t* signal_station_create(signal_station_t* station);
void signal_station_destry(signal_station_t* station, bool all);
void signal_station_reset(signal_station_t* station);
static inline uint16_t signal_station_length(signal_station_t* station) {
	return station->times.length / station->time_length;
}
static inline uint16_t signal_station_size(signal_station_t* station) {
	return stack_buffer_size(&station->times) / station->time_length;
}
void signal_send(signal_station_t* station);
void signal_receive(signal_station_t* station);
void signal_receive_next(signal_station_t* station);


#endif /* SIGNAL_H_ */
