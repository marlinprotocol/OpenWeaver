#include <marlin/multicast/MulticastClientWrapper.h>

// void  did_recv_message (
// 	char* message,
// 	uint64_t message_length,
// 	char* channel,
// 	uint64_t channel_length,
// 	uint64_t message_id) {

// }

void did_recv_message() {
}

int main() {
	MulticastClientDelegate_t *mc_d;
	mc_d->did_recv_message = did_recv_message;

	MulticastClientWrapper_t *m1 = create_multicastclientwrapper();
	setDelegate(m1, mc_d);
	// MulticastClientWrapper m2 = create_multicastclient(options);

	// set delegate's void functions
	// set
}
