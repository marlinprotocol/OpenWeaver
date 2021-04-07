#include <spdlog/spdlog.h>

#define MARLIN_LOG_DEBUG(fmt, ...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(),"{}:{} {}() ###: " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MARLIN_LOG_DEBUG_0(fmt) SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(),"{}:{} {}() !!!. " fmt,  __FILE__, __LINE__, __FUNCTION__)

#define MARLIN_LOG_TRACE(fmt, ...) SPDLOG_LOGGER_TRACE(spdlog::default_logger_raw(),"{}:{} {}() ###: " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MARLIN_LOG_TRACE_0(fmt) SPDLOG_LOGGER_TRACE(spdlog::default_logger_raw(),"{}:{} {}() !!!. " fmt,  __FILE__, __LINE__, __FUNCTION__)

#define MARLIN_LOG_INFO(fmt, ...) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(),"{}:{} {}() ###: " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MARLIN_LOG_INFO_0(fmt) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(),"{}:{} {}() !!!. " fmt,  __FILE__, __LINE__, __FUNCTION__)


