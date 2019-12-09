#ifndef MARLIN_MULTICAST_MULTICASTCLIENTWRAPPER_H
#define MARLIN_MULTICAST_MULTICASTCLIENTWRAPPER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MarlinMulticastClientWrapper MarlinMulticastClientWrapper_t;

typedef void (*type_did_recv_message_func) (
	MarlinMulticastClientWrapper_t* mc_w,
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id);

typedef void (*type_did_subscribe_func) (
	MarlinMulticastClientWrapper_t* mc_w,
	const char* channel,
	uint64_t channel_length);

typedef void (*type_did_unsubscribe_func) (
	MarlinMulticastClientWrapper_t* mc_w,
	const char* channel,
	uint64_t channel_length);

struct MarlinMulticastClientDelegate {
	type_did_recv_message_func did_recv_message;
	type_did_subscribe_func did_subscribe;
	type_did_unsubscribe_func did_unsubscribe;
};

typedef struct MarlinMulticastClientDelegate MarlinMulticastClientDelegate_t;

MarlinMulticastClientDelegate_t* marlin_multicast_create_multicastclientdelegate();

void marlin_multicast_set_did_recv_message(MarlinMulticastClientDelegate_t *mc_d, type_did_recv_message_func f);
void marlin_multicast_set_did_subscribe(MarlinMulticastClientDelegate_t *mc_d, type_did_subscribe_func f);
void marlin_multicast_set_did_unsubscribe(MarlinMulticastClientDelegate_t *mc_d, type_did_unsubscribe_func f);


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
