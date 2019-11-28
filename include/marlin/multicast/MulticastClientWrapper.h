/*
	test if the multicastwrapper object works when passed in
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MulticastClientWrapper;
typedef struct MulticastClientWrapper MulticastClientWrapper_t;

struct MulticastClientDelegate {
	void (*did_recv_message) (
		const char* message,
		uint64_t message_length,
		const char* channel,
		uint64_t channel_length,
	 	uint64_t message_id
	 );


	// to be implemented
	// void (*did_recv_message) (
	// 	MulticastClientWrapper_t* mc_w,
	// 	const char* message,
	// 	const char* channel,
	// 	uint64_t message_id
	// );

	void (*did_subscribe) (
		MulticastClientWrapper_t* mc_w,
		const char* channel
	);

	void (*did_unsubscribe) (
		MulticastClientWrapper_t* mc_w,
		const char* channel
	);
};

typedef struct MulticastClientDelegate MulticastClientDelegate_t;

MulticastClientDelegate_t* create_multicastclientdelegate();
MulticastClientWrapper_t* create_multicastclientwrapper(char* beacon_addr, char* discovery_addr, char* pubsub_addr);
void setDelegate(MulticastClientWrapper_t* mc_w, MulticastClientDelegate_t* mc_d);


// to be implemented

void send_message_on_channel(MulticastClientWrapper_t* mc_w, char *channel, char *message, uint64_t size);
void subscribe_to_channel(MulticastClientWrapper_t* mc_w, char *channel);
void unsubscribe_from_channel(MulticastClientWrapper_t* mc_w, char *channel);

int run_event_loop();

#ifdef __cplusplus
}
#endif
