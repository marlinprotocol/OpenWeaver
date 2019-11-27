#ifdef __cplusplus
extern "C" {
#endif

struct MulticastClientDelegate {
	// void (*did_recv_message)(
	// 	char* message,
	// 	uint64_t message_length,
	// 	char* channel,
	// 	uint64_t channel_length,
	//  	uint64_t message_id
	//  );

	void (*did_recv_message)();
};
typedef struct MulticastClientDelegate MulticastClientDelegate_t;

struct MulticastClientWrapper;
typedef struct MulticastClientWrapper MulticastClientWrapper_t;

MulticastClientWrapper_t* create_multicastclientwrapper();
void setDelegate(MulticastClientWrapper_t* mc_w, MulticastClientDelegate_t* mc_d);

#ifdef __cplusplus
}
#endif

// void set_delegate(void *client, MulticastDelegate *del);

// void set_delegate(void *client, MulticastDelegate *del) {
// 	(DefaultMulticastClient<MulticastDelegateWrapper> *)client->del = new MulticastDelegateWrapper(del);
// }
