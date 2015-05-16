
#ifndef USER_MEMORY_H_
#define USER_MEMORY_H_


#include "osapi.h"
#include "mem.h"


static inline void* m_malloc(uint32_t size) {
	return (void*) os_zalloc(size);
}

static inline void* m_zalloc(uint32_t size) {
	return (void*) os_zalloc(size);
}

static inline void m_free(void* start) {
	os_free(start);
}

static inline void m_memset(void* start, uint8_t b, uint16_t length) {
	memset(start, b, length);
}

static inline void m_memcpy(void* destination, const void* source, uint16_t length) {
	memcpy(destination, source, length);
}


#endif /* USER_MEMORY_H_ */
