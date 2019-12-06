/*
	TODO:
	1. introduce appropriate error messages on failure of any of the functions
	2. when a channel is added or removed, subscribe/unsubscribe to channel to be called
		unsubscribe to removed channel can happen when a message on old/unintended channel is received
		subscribe to add channel can happen on new cycle for now
*/

#include<marlin/multicast/DefaultMulticastClient.hpp>
#include<marlin/multicast/MulticastClientWrapper.h>
#include<algorithm>

using namespace marlin::multicast;
using namespace marlin::net;

class MarlinMulticastDelegateWrapper {
private:
	// MarlinMulticastClientDelegate_t* mc_del;

public:
	MarlinMulticastClientDelegate_t* mc_del;

	void did_recv_message(
		DefaultMulticastClient<MarlinMulticastDelegateWrapper> &client,
		Buffer &&message,
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

		if (mc_del->did_recv_message != 0) {
			mc_del->did_recv_message(
				reinterpret_cast<MarlinMulticastClientWrapper_t *> (&client),
				message.data(),
				message.size(),
				channel.c_str(),
				channel.size(),
				message_id
			);
		}
	}

	void did_subscribe(
		DefaultMulticastClient<MarlinMulticastDelegateWrapper> &client,
		std::string &channel
	) {
		client.ps.send_message_on_channel(channel, "Hello!", 6);

		if (mc_del->did_subscribe != 0) {
			mc_del->did_subscribe(
				reinterpret_cast<MarlinMulticastClientWrapper_t *> (&client),
				channel.c_str(),
				channel.size()
			);
		}
	}

	void did_unsubscribe(
		DefaultMulticastClient<MarlinMulticastDelegateWrapper> &,
		std::string &
	) {}
};

MarlinMulticastClientDelegate_t* marlin_multicast_create_multicastclientdelegate() {
	MarlinMulticastClientDelegate_t *mc_d;
	mc_d = (__typeof__(mc_d))malloc(sizeof(*mc_d));

	//initializing the function pointers with null
	mc_d->did_recv_message = 0;
	mc_d->did_subscribe = 0;
	mc_d->did_unsubscribe = 0;

	return mc_d;
}

void marlin_multicast_set_did_recv_message(MarlinMulticastClientDelegate_t *mc_d, type_did_recv_message_func f) {

	mc_d->did_recv_message = f;
}

MarlinMulticastClientWrapper_t* marlin_multicast_create_multicastclientwrapper(char* beacon_addr, char* discovery_addr, char* pubsub_addr) {

	// TODO: edit default goldfish, a list of channels through arguement instead?
	DefaultMulticastClientOptions clop1 {
		{"goldfish"},
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	return reinterpret_cast<MarlinMulticastClientWrapper_t *> (new DefaultMulticastClient<MarlinMulticastDelegateWrapper> (clop1));
}

void marlin_multicast_setDelegate(MarlinMulticastClientWrapper_t* mc_w, MarlinMulticastClientDelegate_t* mc_d) {
	MarlinMulticastDelegateWrapper* mc_d_w = new MarlinMulticastDelegateWrapper();
	mc_d_w->mc_del = mc_d;


	DefaultMulticastClient<MarlinMulticastDelegateWrapper> *obj = reinterpret_cast< DefaultMulticastClient<MarlinMulticastDelegateWrapper> *>(mc_w);

	obj->delegate = mc_d_w;
}

void marlin_multicast_send_message_on_channel(MarlinMulticastClientWrapper_t* mc_w, const char *channel, char *message, uint64_t size) {
	DefaultMulticastClient<MarlinMulticastDelegateWrapper> *obj = reinterpret_cast< DefaultMulticastClient<MarlinMulticastDelegateWrapper> *>(mc_w);

	obj->ps.send_message_on_channel(channel, message, size);
}

bool marlin_multicast_add_channel(MarlinMulticastClientWrapper_t* mc_w, const char* channel) {
	DefaultMulticastClient<MarlinMulticastDelegateWrapper> *obj = reinterpret_cast< DefaultMulticastClient<MarlinMulticastDelegateWrapper> *>(mc_w);

	if (std::find(obj->channels.begin(), obj->channels.end(), channel) == obj->channels.end()) {
		obj->channels.push_back(channel);
		return true;
	}

	return false;
}

bool marlin_multicast_remove_channel(MarlinMulticastClientWrapper_t* mc_w, const char* channel) {
	DefaultMulticastClient<MarlinMulticastDelegateWrapper> *obj = reinterpret_cast< DefaultMulticastClient<MarlinMulticastDelegateWrapper> *>(mc_w);

	auto it = std::find(obj->channels.begin(), obj->channels.end(), channel);
	if (it != obj->channels.end()) {
		obj->channels.erase(it);
		return true;
	}

	return false;
}

int marlin_multicast_run_event_loop() {
	return DefaultMulticastClient<MarlinMulticastDelegateWrapper>::run_event_loop();
}
