#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/http/HttpPost.hpp>

#include <spdlog/spdlog.h>
#include <rapidjson/document.h>
#include <functional>
#include <string>
#include <map>



namespace marlin{
namespace beacon{

//! Checks that s is an address as the subgraph returns it: 0x followed by exactly 40 hex digits
inline bool is_hex_address(std::string const& s) {
	if(s.size() != 42 || s[0] != '0' || s[1] != 'x') {
		return false;
	}
	for(size_t i = 2; i < s.size(); i++) {
		auto c = s[i];
		if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
			return false;
		}
	}
	return true;
}

//! Periodically fetches the client keys of the clusters in the staking subgraph.
//! Clusters whose client key is not in the subgraph are not connected to.
struct Grapher {
	asyncio::Timer t;
	//! Request in flight, if any
	asyncio::HttpPostHandle req;
	std::map< std::string, std::string> clientkey_id_map;

	//! Called after every successful update of clientkey_id_map
	std::function<void()> on_update;

	template<typename... Args>
	Grapher(Args&&...) : t(this) {
		t.template start<Grapher, &Grapher::query>(0, 300000);
	}

	Grapher(Grapher const&) = delete;
	Grapher& operator=(Grapher const&) = delete;

	~Grapher() {
		req.cancel();
	}

	void query() {
		auto url = asyncio::resolve_subgraph_url("/subgraphs/name/marlinprotocol/staking-arb1");
		SPDLOG_DEBUG("Grapher: querying {}", asyncio::redact_url(url));
		// The callback captures this, the request is cancelled in the destructor
		req.cancel();
		req = asyncio::http_post(
			url,
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4\\\"}){clientKey, id}}\"}",
			[this, url](asyncio::HttpResponse&& res) {
				did_query(url, std::move(res));
			}
		);
	}

	//! Reads the clientKey -> id pairs of a subgraph reply into m.
	//! Entries where either is not a 20 byte hex address are skipped, the ids are later decoded into 20 byte arrays.
	//! Returns false if the reply does not have a clusters array.
	static bool parse_clusters(std::string const& body, std::map<std::string, std::string>& m) {
		rapidjson::Document JData;
		JData.Parse(body.data(), body.size());
		if(JData.HasParseError() || !JData.IsObject() || !JData.HasMember("data") || !JData["data"].IsObject()
			|| !JData["data"].HasMember("clusters") || !JData["data"]["clusters"].IsArray()) {
			return false;
		}

		auto& clusters = JData["data"]["clusters"];
		for(rapidjson::SizeType i = 0; i < clusters.Size(); i++){
			auto& cluster = clusters[i];
			if(!cluster.IsObject() || !cluster.HasMember("clientKey") || !cluster["clientKey"].IsString()
				|| !cluster.HasMember("id") || !cluster["id"].IsString()) {
				continue;
			}
			std::string client_key(cluster["clientKey"].GetString(), cluster["clientKey"].GetStringLength());
			std::string id(cluster["id"].GetString(), cluster["id"].GetStringLength());
			if(!is_hex_address(client_key) || !is_hex_address(id)) {
				SPDLOG_WARN("Grapher: skipping cluster with malformed clientKey or id: {}, {}", client_key.substr(0, 50), id.substr(0, 50));
				continue;
			}
			m[client_key] = id;
		}

		return true;
	}

private:
	void did_query(std::string const& url, asyncio::HttpResponse&& res) {
		if(!res.error.empty()) {
			SPDLOG_ERROR("Grapher: subgraph query to {} failed: {}", asyncio::redact_url(url), res.error);
			return;
		}
		if(res.status != 200) {
			SPDLOG_ERROR("Grapher: subgraph query to {} returned HTTP {}: {}", asyncio::redact_url(url), res.status, res.body.substr(0, 200));
			return;
		}

		std::map<std::string, std::string> m;
		if(!parse_clusters(res.body, m)) {
			SPDLOG_ERROR("Grapher: unexpected subgraph response: {}", res.body.substr(0, 200));
			return;
		}
		clientkey_id_map = std::move(m);
		SPDLOG_INFO("Grapher: {} clusters in the staking subgraph", clientkey_id_map.size());

		if(on_update) {
			on_update();
		}
	}
};

}
}
