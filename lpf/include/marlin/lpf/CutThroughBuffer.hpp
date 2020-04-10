#ifndef MARLIN_LPF_CTB_HPP
#define MARLIN_LPF_CTB_HPP

#include <marlin/core/Buffer.hpp>

namespace marlin {
namespace lpf {

class CutThroughBuffer {
	bool cut_through = false;
	uint64_t length = 0;
	uint64_t size = 0;

public:
	uint16_t id = 0;

	template<typename Delegate>
	int did_recv_bytes(
		Delegate &delegate,
		core::Buffer &&bytes
	) {
		if(bytes.size() == 0) return 0;

		if(cut_through == false) { // Read length
			if(bytes.size() + size < 8) { // Partial length
				for(size_t i = 0; i < bytes.size(); i++) {
					length = (length << 8) | bytes.data()[i];
				}
				size += bytes.size();
			} else { // Full length
				for(size_t i = 0; i < 8 - size; i++) {
					length = (length << 8) | bytes.data()[i];
				}
				bytes.cover_unsafe(8 - size);

				if(length > 5000000) { // Abort on big message, DoS prevention
					SPDLOG_ERROR("Message too big: {}", length);
					return -1;
				}

				// Prepare to process message
				delegate.cut_through_recv_start(id, length);
				cut_through = true;
				size = 0;

				// Process remaining bytes
				return did_recv_bytes(delegate, std::move(bytes));
			}
		} else { // Cut through message
			if(bytes.size() + size < length) { // Partial message
				size += bytes.size();
				auto res = delegate.cut_through_recv_bytes(id, std::move(bytes));
				if(res < 0) {
					return -2;
				}
			} else { // Full message
				core::Buffer tbytes(length - size);
				tbytes.write(0, bytes.data(), length - size);
				auto res = delegate.cut_through_recv_bytes(id, std::move(tbytes));
				if(res < 0) {
					return -2;
				}
				delegate.cut_through_recv_end(id);

				bytes.cover_unsafe(length - size);

				// Prepare to process length
				cut_through = false;
				size = 0;
				length = 0;

				// Process remaining bytes
				return did_recv_bytes(delegate, std::move(bytes));
			}
		}

		return 0;
	}
};

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_CTB_HPP
