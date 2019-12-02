#include <stdio.h>
#include <marlin/multicast/MulticastClientWrapper.h>

void  did_recv_message (
	MulticastClientWrapper_t* mc_w,
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id) {

	printf("C message:%s, message_length: %llu channel:%s, message_id: %llu \n", message, message_length, channel, message_id);
}

void did_subscribe (
		MulticastClientWrapper_t* mc_w,
		const char* channel,
		uint64_t channel_length
	) {
	printf("C message did subscribe to channel: %s\n", channel);
	send_message_on_channel(mc_w, channel, "C Hello!", 8);
}

int main() {
	MulticastClientDelegate_t *mc_d = create_multicastclientdelegate();

	mc_d->did_recv_message = did_recv_message;
	mc_d->did_subscribe = did_subscribe;

	MulticastClientWrapper_t *m2 = create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:7002", "127.0.0.1:7000");
	setDelegate(m2, mc_d);

	MulticastClientWrapper_t *m1 = create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:8002", "127.0.0.1:8000");
	setDelegate(m1, mc_d);

	printf("%d\n", add_channel(m1, "blackfish"));
	printf("%d\n", remove_channel(m1, "blackfish"));
	// printf("%d\n", remove_channel(m1, "blackfish"));

	return run_event_loop();
}
