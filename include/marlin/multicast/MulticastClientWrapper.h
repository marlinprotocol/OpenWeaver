#ifndef MARLIN_MULTICAST_MULTICASTCLIENTWRAPPER_H
#define MARLIN_MULTICAST_MULTICASTCLIENTWRAPPER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MarlinMulticastClientWrapper MarlinMulticastClientWrapper_t;

struct MarlinMulticastClientDelegate {
	void (*did_recv_message) (
		MarlinMulticastClientWrapper_t* mc_w,
		const char* message,
		uint64_t message_length,
		const char* channel,
		uint64_t channel_length,
	 	uint64_t message_id
	 );

	void (*did_subscribe) (
		MarlinMulticastClientWrapper_t* mc_w,
		const char* channel,
		uint64_t channel_length
	);

	void (*did_unsubscribe) (
		MarlinMulticastClientWrapper_t* mc_w,
		const char* channel,
		uint64_t channel_length
	);
};

typedef struct MarlinMulticastClientDelegate MarlinMulticastClientDelegate_t;

MarlinMulticastClientDelegate_t* marlin_multicast_create_multicastclientdelegate();
MarlinMulticastClientWrapper_t* marlin_multicast_create_multicastclientwrapper(char* beacon_addr, char* discovery_addr, char* pubsub_addr);
void marlin_multicast_setDelegate(MarlinMulticastClientWrapper_t* mc_w, MarlinMulticastClientDelegate_t* mc_d);

void marlin_multicast_send_message_on_channel(MarlinMulticastClientWrapper_t* mc_w, const char *channel, char *message, uint64_t size);

bool marlin_multicast_add_channel(MarlinMulticastClientWrapper_t* mc_w, const char* channel);
bool marlin_multicast_remove_channel(MarlinMulticastClientWrapper_t* mc_w, const char* channel);

int marlin_multicast_run_event_loop();

#ifdef __cplusplus
}
#endif

#endif // MARLIN_MULTICAST_MULTICASTCLIENTWRAPPER_H
