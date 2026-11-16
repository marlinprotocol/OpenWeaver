#ifndef MARLIN_ASYNCIO_HTTP_HTTPPOST_HPP
#define MARLIN_ASYNCIO_HTTP_HTTPPOST_HPP

#include <uv.h>
#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace marlin {
namespace asyncio {

/// Result of an HTTP request. `error` is empty when the request completed,
/// `status` is the HTTP status code in that case.
struct HttpResponse {
	long status = 0;
	std::string body;
	std::string error;
};

/// Handle of a request started with http_post.
///
/// An object whose callback captures `this` must keep the handle and call
/// cancel() in its destructor, otherwise the callback runs on a destroyed
/// object if the request is still in flight.
class HttpPostHandle {
	std::shared_ptr<std::atomic<bool>> cancelled;
public:
	HttpPostHandle() = default;
	HttpPostHandle(std::shared_ptr<std::atomic<bool>> cancelled) : cancelled(std::move(cancelled)) {}

	/// Cancels the request. Must be called on the loop thread. The callback
	/// is not called after this, and the transfer is aborted if it is running.
	/// Does nothing if the request has completed or the handle is empty.
	void cancel() {
		if(cancelled) {
			cancelled->store(true);
			cancelled.reset();
		}
	}
};

/// Largest response body http_post accepts. A request whose response is
/// larger fails with an error. Subgraph replies are a few tens of KB.
constexpr size_t http_post_max_response_size = 16 * 1024 * 1024;

namespace detail {

struct HttpPostReq {
	uv_work_t work;
	std::string url;
	std::string body;
	std::function<void(HttpResponse&&)> cb;
	HttpResponse res;
	bool too_large = false;
	// Shared with the HttpPostHandle of the request
	std::shared_ptr<std::atomic<bool>> cancelled = std::make_shared<std::atomic<bool>>(false);
};

inline int http_post_xferinfo_cb(void* userdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
	auto& req = *(HttpPostReq*)userdata;
	// A non zero value makes libcurl abort the transfer
	return req.cancelled->load() ? 1 : 0;
}

inline size_t http_post_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
	auto& req = *(HttpPostReq*)userdata;
	auto& out = req.res.body;
	// libcurl passes size == 1 and at most CURL_MAX_WRITE_SIZE bytes
	auto num = size * nmemb;
	if(num > http_post_max_response_size - out.size()) {
		// Returning less than num makes libcurl abort the transfer
		req.too_large = true;
		return 0;
	}
	out.append(ptr, num);
	return num;
}

inline void http_post_global_init() {
	static std::once_flag flag;
	std::call_once(flag, []() {
		curl_global_init(CURL_GLOBAL_DEFAULT);
	});
}

}  // namespace detail

/// Returns the URL with the path after the host removed, for logging URLs
/// that may carry an API key.
inline std::string redact_url(std::string const& url) {
	auto scheme = url.find("://");
	auto start = scheme == std::string::npos ? 0 : scheme + 3;
	auto path = url.find('/', start);
	if(path == std::string::npos) {
		return url;
	}
	return url.substr(0, path) + "/...";
}

/// POSTs a JSON body to an http or https URL. The request runs on the libuv
/// thread pool and `cb` is called on the loop thread when it completes.
/// Proxy environment variables (https_proxy etc.) are honoured by libcurl.
/// `cb` is not called if the request is cancelled through the returned handle.
inline HttpPostHandle http_post(
	std::string url,
	std::string body,
	std::function<void(HttpResponse&&)> cb,
	uv_loop_t* loop = uv_default_loop()
) {
	detail::http_post_global_init();

	auto* req = new detail::HttpPostReq();
	req->work.data = req;
	req->url = std::move(url);
	req->body = std::move(body);
	req->cb = std::move(cb);
	HttpPostHandle handle(req->cancelled);

	auto res = uv_queue_work(loop, &req->work, [](uv_work_t* work) {
		auto& req = *(detail::HttpPostReq*)work->data;
		if(req.cancelled->load()) {
			return;
		}

		CURL* curl = curl_easy_init();
		if(curl == nullptr) {
			req.res.error = "curl_easy_init failed";
			return;
		}

		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");

		char errbuf[CURL_ERROR_SIZE] = {0};
		curl_easy_setopt(curl, CURLOPT_URL, req.url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req.body.size());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, detail::http_post_write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &req);
		// Refuses early when the server announces the size, the write callback covers the rest
		curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)http_post_max_response_size);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
		// libcurl may be built with file, ftp and more, the URL and redirects are limited to http and https
#if LIBCURL_VERSION_NUM >= 0x075500
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
		curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
		curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		// Polled by libcurl about once a second and on activity, stops the transfer of a cancelled request
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, detail::http_post_xferinfo_cb);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &req);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

		auto code = curl_easy_perform(curl);
		if(req.too_large || code == CURLE_FILESIZE_EXCEEDED) {
			req.res.error = "response is larger than " + std::to_string(http_post_max_response_size) + " bytes";
			req.res.body.clear();
			req.res.body.shrink_to_fit();
		} else if(code != CURLE_OK) {
			req.res.error = errbuf[0] != 0 ? std::string(errbuf) : std::string(curl_easy_strerror(code));
		} else {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &req.res.status);
		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}, [](uv_work_t* work, int status) {
		auto* req = (detail::HttpPostReq*)work->data;
		if(status != 0 && req->res.error.empty()) {
			req->res.error = uv_strerror(status);
		}
		// The owner of the callback may be gone once the request is cancelled
		if(!req->cancelled->load()) {
			req->cb(std::move(req->res));
		}
		delete req;
	});

	if(res != 0) {
		HttpResponse r;
		r.error = uv_strerror(res);
		auto cb = std::move(req->cb);
		delete req;
		cb(std::move(r));
		return HttpPostHandle();
	}

	return handle;
}

/// URL of the Marlin staking subgraph set for this process. Empty until
/// set_subgraph_url is called; while empty, callers fall back to their
/// built-in graph.marlin.pro paths.
inline std::string& subgraph_url_override() {
	static std::string url;
	return url;
}

/// Sets the staking subgraph URL for this process, e.g. from a --subgraph-url
/// flag. An empty string keeps the current value.
inline void set_subgraph_url(std::string const& url) {
	if(!url.empty()) {
		subgraph_url_override() = url;
	}
}

/// Resolves the subgraph URL for a request. If a URL was set with
/// set_subgraph_url, it is used. Otherwise `path_or_url`
/// is used as is when it is a full URL, or as a path on graph.marlin.pro.
inline std::string resolve_subgraph_url(std::string const& path_or_url) {
	auto& o = subgraph_url_override();
	if(!o.empty()) {
		return o;
	}
	if(path_or_url.rfind("http://", 0) == 0 || path_or_url.rfind("https://", 0) == 0) {
		return path_or_url;
	}
	return "https://graph.marlin.pro" + path_or_url;
}

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_HTTP_HTTPPOST_HPP
