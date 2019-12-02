/*
	TODO:
	1. for the delegated functions, a new object of multicastclientwrapper needs to be created everytime. is it time consuming
		CDefaultMulticast.cpp line 37, 58
	2. introduce appropriate error messages on failure of any of the functions
	3. when a channel is added or removed, subscribe/unsubscribe to channel to be called
		unsubscribe to removed channel can happen when a message on old/unintended channel is received
		subscribe to add channel can happen on new cycle for now
*/

#include<marlin/multicast/DefaultMulticastClient.hpp>
#include<marlin/multicast/MulticastClientWrapper.h>
#include<algorithm>

using namespace marlin::multicast;
using namespace marlin::net;


struct MulticastClientWrapper {
	void *obj;
};

class MulticastDelegateWrapper {
private:
	// MulticastClientDelegate_t* mc_del;

public:
	MulticastClientDelegate_t* mc_del;

	void did_recv_message(
		DefaultMulticastClient<MulticastDelegateWrapper> &client,
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

		// TODO: HACK! recursive assignments, freeing memory?, time consuming to perform everytime
		MulticastClientWrapper_t *mc_w;
		mc_w = (__typeof__(mc_w))malloc(sizeof(*mc_w));
		mc_w->obj = &client;

		mc_del->did_recv_message(
			mc_w,
			message.data(),
			message.size(),
			channel.c_str(),
			channel.size(),
			message_id
		);
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegateWrapper> &client,
		std::string &channel
	) {
		client.ps.send_message_on_channel(channel, "Hello!", 6);

		// TODO: HACK! recursive assignments, freeing memory?, time consuming to perform everytime
		MulticastClientWrapper_t *mc_w;
		mc_w = (__typeof__(mc_w))malloc(sizeof(*mc_w));
		mc_w->obj = &client;

		mc_del->did_subscribe(
			mc_w,
			channel.c_str(),
			channel.size()
		);
	}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegateWrapper> &,
		std::string &
	) {}
};

MulticastClientDelegate_t* create_multicastclientdelegate() {
	MulticastClientDelegate_t *mc_d;
	mc_d = (__typeof__(mc_d))malloc(sizeof(*mc_d));

	return mc_d;
}

MulticastClientWrapper_t* create_multicastclientwrapper(char* beacon_addr, char* discovery_addr, char* pubsub_addr) {

	// TODO: edit default goldfish, a list of channels through arguement instead?
	DefaultMulticastClientOptions clop1 {
		{"goldfish"},
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	MulticastClientWrapper_t *mc_w;
	mc_w = (__typeof__(mc_w))malloc(sizeof(*mc_w));

	mc_w->obj = new DefaultMulticastClient<MulticastDelegateWrapper> (clop1);

	return mc_w;
}

void setDelegate(MulticastClientWrapper_t* mc_w, MulticastClientDelegate_t* mc_d) {
	MulticastDelegateWrapper* mc_d_w = new MulticastDelegateWrapper();
	mc_d_w->mc_del = mc_d;


	DefaultMulticastClient<MulticastDelegateWrapper> *obj;
	obj = reinterpret_cast< DefaultMulticastClient<MulticastDelegateWrapper> *>(mc_w->obj);

	obj->delegate = mc_d_w;

}

void send_message_on_channel(MulticastClientWrapper_t* mc_w, const char *channel, char *message, uint64_t size) {
	DefaultMulticastClient<MulticastDelegateWrapper> *obj;
	obj = reinterpret_cast< DefaultMulticastClient<MulticastDelegateWrapper> *>(mc_w->obj);

	obj->ps.send_message_on_channel(channel, message, size);
}

bool add_channel(MulticastClientWrapper_t* mc_w, const char* channel) {
	DefaultMulticastClient<MulticastDelegateWrapper> *obj;
	obj = reinterpret_cast< DefaultMulticastClient<MulticastDelegateWrapper> *>(mc_w->obj);

	if (std::find(obj->channels.begin(), obj->channels.end(), channel) == obj->channels.end()) {
		obj->channels.push_back(channel);
		return true;
	}

	return false;
}

bool remove_channel(MulticastClientWrapper_t* mc_w, const char* channel) {
	DefaultMulticastClient<MulticastDelegateWrapper> *obj;
	obj = reinterpret_cast< DefaultMulticastClient<MulticastDelegateWrapper> *>(mc_w->obj);

	auto it = std::find(obj->channels.begin(), obj->channels.end(), channel);
	if (it != obj->channels.end()) {
		obj->channels.erase(it);
		return true;
	}

	return false;
}

int run_event_loop() {
	return DefaultMulticastClient<MulticastDelegateWrapper>::run_event_loop();
}
