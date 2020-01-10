#ifndef MARLIN_MULTICAST_MARLINMULTICASTCLIENT_H
#define MARLIN_MULTICAST_MARLINMULTICASTCLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MarlinMulticastClient MarlinMulticastClient_t;
typedef struct MarlinMulticastClientDelegate MarlinMulticastClientDelegate_t;

// Delegate
MarlinMulticastClientDelegate_t* marlin_multicast_clientdelegate_create();
void marlin_multicast_clientdelegate_destroy(
	MarlinMulticastClientDelegate_t* delegate
);

typedef void (*did_recv_message_func) (
	MarlinMulticastClient_t* client,
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id
);
void marlin_multicast_clientdelegate_set_did_recv_message(
	MarlinMulticastClientDelegate_t *delegate,
	did_recv_message_func f
);

typedef void (*did_subscribe_func) (
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_length
);
void marlin_multicast_clientdelegate_set_did_subscribe(
	MarlinMulticastClientDelegate_t *delegate,
	did_subscribe_func f
);

typedef void (*did_unsubscribe_func) (
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_length
);
void marlin_multicast_clientdelegate_set_did_unsubscribe(
	MarlinMulticastClientDelegate_t *delegate,
	did_unsubscribe_func f
);

// Client
MarlinMulticastClient_t* marlin_multicast_client_create(
	uint8_t* static_sk,
	char* beacon_addr,
	char* discovery_addr,
	char* pubsub_addr
);
void marlin_multicast_client_destroy(MarlinMulticastClient_t* client);

void marlin_multicast_client_set_delegate(
	MarlinMulticastClient_t* client,
	MarlinMulticastClientDelegate_t* delegate
);

void marlin_multicast_client_send_message_on_channel(
	MarlinMulticastClient_t* client,
	const char *channel,
	uint64_t channel_size,
	char *message,
	uint64_t size
);

bool marlin_multicast_client_add_channel(
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_size
);
bool marlin_multicast_client_remove_channel(
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_size
);

// Util
int marlin_multicast_run_event_loop();

void marlin_multicast_create_keypair(uint8_t* static_pk, uint8_t* static_sk);

#ifdef __cplusplus
}
#endif

#endif // MARLIN_MULTICAST_MARLINMULTICASTCLIENT_H
