#include <spdlog/spdlog.h>
#include <marlin/core/Buffer.hpp>

using namespace marlin::core;


struct EmptyTerminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	EmptyTerminal(Args&&...) {}

	int did_recv(auto&&, Buffer&&) {
		SPDLOG_INFO("Did recv: EmptyTerminal");
		return 0;
	}
};
