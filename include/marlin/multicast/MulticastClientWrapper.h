#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MulticastClientDelegate {
	void (*did_recv_message)(
		const char* message,
		uint64_t message_length,
		const char* channel,
		uint64_t channel_length,
	 	uint64_t message_id
	 );
};
typedef struct MulticastClientDelegate MulticastClientDelegate_t;

struct MulticastClientWrapper;
typedef struct MulticastClientWrapper MulticastClientWrapper_t;

MulticastClientDelegate_t* create_multicastclientdelegate();
MulticastClientWrapper_t* create_multicastclientwrapper(char* beacon_addr, char* discovery_addr, char* pubsub_addr);
void setDelegate(MulticastClientWrapper_t* mc_w, MulticastClientDelegate_t* mc_d);

int run_event_loop();

#ifdef __cplusplus
}
#endif
