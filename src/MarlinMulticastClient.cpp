/*
	TODO:
	1. introduce appropriate error messages on failure of any of the functions
	2. when a channel is added or removed, subscribe/unsubscribe to channel to be called
		unsubscribe to removed channel can happen when a message on old/unintended channel is received
		subscribe to add channel can happen on new cycle for now
*/

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/multicast/MarlinMulticastClient.h>
#include <algorithm>

using namespace marlin::multicast;
using namespace marlin::net;

// Delegate
struct MarlinMulticastClientDelegate {
	did_recv_message_func m_did_recv_message = 0;
	did_subscribe_func m_did_subscribe = 0;
	did_unsubscribe_func m_did_unsubscribe = 0;

	void did_recv_message(
		DefaultMulticastClient<MarlinMulticastClientDelegate> &client,
		Buffer &&message,
		Buffer &&,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Did recv message: {}",
			std::string(message.data(), message.data() + message.size())
		);

		SPDLOG_INFO(
			"channel: {}, message_id: {}",
			channel,
			message_id
		);

		// TODO: Should check or assert?
		if (this->m_did_recv_message != 0) {
			this->m_did_recv_message(
				reinterpret_cast<MarlinMulticastClient_t *> (&client),
				message.data(),
				message.size(),
				channel.c_str(),
				channel.size(),
				message_id
			);
		}
	}

	void did_subscribe(
		DefaultMulticastClient<MarlinMulticastClientDelegate> &client,
		std::string &channel
	) {
		client.ps.send_message_on_channel(channel, "Hello!", 6);

		if (this->m_did_subscribe != 0) {
			this->m_did_subscribe(
				reinterpret_cast<MarlinMulticastClient_t *> (&client),
				channel.c_str(),
				channel.size()
			);
		}
	}

	void did_unsubscribe(
		DefaultMulticastClient<MarlinMulticastClientDelegate> &,
		std::string &
	) {}
};


// Delgate impl
MarlinMulticastClientDelegate_t* marlin_multicast_clientdelegate_create() {
	return new MarlinMulticastClientDelegate_t();
}

void marlin_multicast_clientdelegate_destroy(MarlinMulticastClientDelegate_t* delegate) {
	delete delegate;
}

void marlin_multicast_clientdelegate_set_did_recv_message(
	MarlinMulticastClientDelegate_t *delegate,
	did_recv_message_func f
) {
	delegate->m_did_recv_message = f;
}

void marlin_multicast_clientdelegate_set_did_subscribe(
	MarlinMulticastClientDelegate_t *delegate,
	did_subscribe_func f
) {
	delegate->m_did_subscribe = f;
}

void marlin_multicast_clientdelegate_set_did_unsubscribe(
	MarlinMulticastClientDelegate_t *delegate,
	did_unsubscribe_func f
) {
	delegate->m_did_unsubscribe = f;
}


// Client impl
MarlinMulticastClient_t* marlin_multicast_client_create(
	uint8_t* static_sk,
	char* beacon_addr,
	char* discovery_addr,
	char* pubsub_addr
) {
	// TODO: edit default goldfish, a list of channels through arguement instead?
	DefaultMulticastClientOptions clop1 {
		static_sk,
		{"goldfish"},
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	return reinterpret_cast<MarlinMulticastClient_t *>(
		new DefaultMulticastClient<MarlinMulticastClientDelegate>(clop1)
	);
}

void marlin_multicast_client_destroy(MarlinMulticastClient_t* client) {
	delete (DefaultMulticastClient<MarlinMulticastClientDelegate>*)client;
}

void marlin_multicast_client_set_delegate(
	MarlinMulticastClient_t* client,
	MarlinMulticastClientDelegate_t* delegate
) {
	auto* obj = reinterpret_cast<
		DefaultMulticastClient<MarlinMulticastClientDelegate>*
	>(client);

	obj->delegate = delegate;
}

void marlin_multicast_client_send_message_on_channel(
	MarlinMulticastClient_t* client,
	const char *channel,
	uint64_t channel_size,
	char *message,
	uint64_t size
) {
	auto* obj = reinterpret_cast<
		DefaultMulticastClient<MarlinMulticastClientDelegate>*
	>(client);

	obj->ps.send_message_on_channel(
		std::string(channel, channel + channel_size),
		message,
		size
	);
}

bool marlin_multicast_client_add_channel(
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_size
) {
	auto* obj = reinterpret_cast<
		DefaultMulticastClient<MarlinMulticastClientDelegate>*
	>(client);

	if (std::find(obj->channels.begin(), obj->channels.end(), std::string(channel, channel + channel_size)) == obj->channels.end()) {
		obj->channels.push_back(std::string(channel, channel + channel_size));
		return true;
	}

	return false;
}

bool marlin_multicast_client_remove_channel(
	MarlinMulticastClient_t* client,
	const char* channel,
	uint64_t channel_size
) {
	auto* obj = reinterpret_cast<
		DefaultMulticastClient<MarlinMulticastClientDelegate>*
	>(client);

	auto it = std::find(obj->channels.begin(), obj->channels.end(), std::string(channel, channel + channel_size));
	if (it != obj->channels.end()) {
		obj->channels.erase(it);
		return true;
	}

	return false;
}

// Util
int marlin_multicast_run_event_loop() {
	return DefaultMulticastClient<MarlinMulticastClientDelegate>::run_event_loop();
}
