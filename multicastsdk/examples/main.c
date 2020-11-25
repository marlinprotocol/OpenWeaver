#include <stdio.h>
#include <marlin/multicast/MarlinMulticastClient.h>

void  did_recv (
	MarlinMulticastClient_t* client,
	const uint8_t* message,
	uint64_t message_length,
	uint16_t channel,
	uint64_t message_id
) {
	printf(
		"C message:%.*s, message_length: %llu, channel:%u, message_id: %llu\n",
		message_length,
		message,
		message_length,
		channel,
		message_id
	);
}

void did_subscribe (
	MarlinMulticastClient_t* client,
	uint16_t channel
) {
	printf(
		"C message did subscribe to channel: %u\n",
		channel
	);

	marlin_multicast_client_send_message_on_channel(
		client,
		channel,
		(uint8_t*)"C Hello!",
		8
	);
}

int main() {
	MarlinMulticastClientDelegate_t *delegate = marlin_multicast_clientdelegate_create();
	marlin_multicast_clientdelegate_set_did_recv(
		delegate,
		did_recv
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
		static_pk2,
		"127.0.0.1:9002",
		"127.0.0.1:7002",
		"127.0.0.1:7000"
	);
	marlin_multicast_client_set_delegate(m2, delegate);

	MarlinMulticastClient_t *m1 = marlin_multicast_client_create(
		static_sk1,
		static_pk1,
		"127.0.0.1:9002",
		"127.0.0.1:8002",
		"127.0.0.1:8000"
	);
	marlin_multicast_client_set_delegate(m1, delegate);

	printf("%d\n", marlin_multicast_client_add_channel(m1, 1));
	printf("%d\n", marlin_multicast_client_remove_channel(m1, 1));

	return marlin_multicast_run_event_loop();
}
