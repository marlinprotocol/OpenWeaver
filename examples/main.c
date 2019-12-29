#include <stdio.h>
#include <marlin/multicast/MarlinMulticastClient.h>

void  did_recv_message (
	MarlinMulticastClient_t* client,
	const char* message,
	uint64_t message_length,
	const char* channel,
	uint64_t channel_length,
	uint64_t message_id
) {
	printf(
		"C message:%.*s, message_length: %llu, channel:%.*s, message_id: %llu\n",
		message_length,
		message,
		message_length,
		channel_length,
		channel,
		message_id
	);
}

void did_subscribe (
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_length
) {
	printf(
		"C message did subscribe to channel: %.*s\n",
		channel_length,
		channel
	);

	marlin_multicast_client_send_message_on_channel(
		client,
		channel,
		channel_length,
		"C Hello!",
		8
	);
}

int main() {
	MarlinMulticastClientDelegate_t *delegate = marlin_multicast_clientdelegate_create();
	marlin_multicast_clientdelegate_set_did_recv_message(
		delegate,
		did_recv_message
	);
	marlin_multicast_clientdelegate_set_did_subscribe(
		delegate,
		did_subscribe
	);

	uint8_t static_sk1[32];
	uint8_t static_pk1[32];
	marlin_multicast_create_keypair(static_pk1, static_sk1);

	uint8_t static_sk2[32];
	uint8_t static_pk2[32];
	marlin_multicast_create_keypair(static_pk2, static_sk2);

	MarlinMulticastClient_t *m2 = marlin_multicast_client_create(
		static_sk2,
		"127.0.0.1:9002",
		"127.0.0.1:7002",
		"127.0.0.1:7000"
	);
	marlin_multicast_client_set_delegate(m2, delegate);

	MarlinMulticastClient_t *m1 = marlin_multicast_client_create(
		static_sk1,
		"127.0.0.1:9002",
		"127.0.0.1:8002",
		"127.0.0.1:8000"
	);
	marlin_multicast_client_set_delegate(m1, delegate);

	printf("%d\n", marlin_multicast_client_add_channel(m1, "blackfish", 9));
	printf("%d\n", marlin_multicast_client_remove_channel(m1, "blackfish", 9));

	return marlin_multicast_run_event_loop();
}
