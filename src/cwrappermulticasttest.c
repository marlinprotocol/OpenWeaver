#include <stdio.h>
#include <marlin/multicast/MulticastClientWrapper.h>

void  did_recv_message (
	MarlinMulticastClientWrapper_t* mc_w,
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id) {

	//message, channel to be read only till there provided lengths message_length & channel length

	// printf("C message:%s, message_length: %llu channel:%s, message_id: %llu \n", message, message_length, channel, message_id);
}

void did_subscribe (
	MarlinMulticastClientWrapper_t* mc_w,
	const char* channel,
	uint64_t channel_length) {

	//channel to be read only till its provided length channel length

	// printf("C message did subscribe to channel: %s\n", channel);
	marlin_multicast_send_message_on_channel(mc_w, channel, "C Hello!", 8);
}

int main() {
	MarlinMulticastClientDelegate_t *mc_d = marlin_multicast_create_multicastclientdelegate();

	marlin_multicast_set_did_recv_message(mc_d, did_recv_message);

	mc_d->did_subscribe = did_subscribe;

	MarlinMulticastClientWrapper_t *m2 = marlin_multicast_create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:7002", "127.0.0.1:7000");
	marlin_multicast_setDelegate(m2, mc_d);

	MarlinMulticastClientWrapper_t *m1 = marlin_multicast_create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:8002", "127.0.0.1:8000");
	marlin_multicast_setDelegate(m1, mc_d);

	printf("%d\n", marlin_multicast_add_channel(m1, "blackfish"));
	printf("%d\n", marlin_multicast_remove_channel(m1, "blackfish"));
	// printf("%d\n", marlin_multicast_remove_channel(m1, "blackfish"));

	return marlin_multicast_run_event_loop();
}
