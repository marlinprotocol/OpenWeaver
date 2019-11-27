#include <stdio.h>
#include <marlin/multicast/MulticastClientWrapper.h>

void  did_recv_message (
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id) {

	printf("C message:%s, channel:%s, message_id: %llu \n", message, channel, message_id);
}

int main() {
	MulticastClientDelegate_t *mc_d = create_multicastclientdelegate();

	mc_d->did_recv_message = did_recv_message;

	MulticastClientWrapper_t *m2 = create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:7002", "127.0.0.1:7000");
	setDelegate(m2, mc_d);

	MulticastClientWrapper_t *m1 = create_multicastclientwrapper("127.0.0.1:9002", "127.0.0.1:8002", "127.0.0.1:8000");
	setDelegate(m1, mc_d);

	return run_event_loop();
}
