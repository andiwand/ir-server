
#ifndef INCLUDE_UPDATE_H_
#define INCLUDE_UPDATE_H_


#include "c_types.h"


#define UPDATE_TIMEOUT 10000
#define UPDATE_FINISH_DELAY 500

#define UPDATE_SIZE 0x3d000
#define UPDATE_ADDR_USER1 0x01000
#define UPDATE_ADDR_USER2 0x41000


typedef enum update_state update_state_t;


enum update_state {
	UPDATE_READY,
	UPDATE_BUSY,
	UPDATE_DONE
};


void update_init(uint16 port);

static void update_listen(void* arg);
static void update_discon(void* arg);
static void update_timeout(void* arg);
static void update_recv(void* arg, char* data, unsigned short len);
static void update_finish();


#endif /* INCLUDE_UPDATE_H_ */
