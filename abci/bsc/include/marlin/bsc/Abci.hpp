#ifndef MARLIN_BSC_ABCI
#define MARLIN_BSC_ABCI

#include <uv.h>
#include <spdlog/spdlog.h>

namespace marlin {
namespace bsc {

template<typename DelegateType>
class Abci {
public:
	using SelfType = Abci<DelegateType>;
private:
	uv_pipe_t* pipe;

	static void connect_cb(uv_connect_t* req, int status) {
		if(status < 0) {
			SPDLOG_ERROR("Abci connection error: {}", status);
			return;
		}

		auto* abci = (SelfType*)req->data;
		delete req;
		abci->delegate->did_connect(*abci);
	}
public:
	DelegateType* delegate;

	Abci(std::string datadir) {
		pipe = new uv_pipe_t();
		pipe->data = this;
		uv_pipe_init(uv_default_loop(), pipe, 0);

		auto req = new uv_connect_t();
		req->data = this;
		uv_pipe_connect(req, pipe, (datadir + "/geth.ipc").c_str(), connect_cb);
	}
};

}  // namespace bsc
}  // namespace marlin

#endif  // MARLIN_BSC_ABCI
