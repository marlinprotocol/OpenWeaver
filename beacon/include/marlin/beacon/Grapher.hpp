#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/http/HttpPost.hpp>

#include <spdlog/spdlog.h>
#include <rapidjson/document.h>
#include <functional>
#include <string>
#include <map>



namespace marlin{
namespace beacon{

//! Periodically fetches the client keys of the clusters in the staking subgraph.
//! Clusters whose client key is not in the subgraph are not connected to.
struct Grapher {
	asyncio::Timer t;
	std::map< std::string, std::string> clientkey_id_map;

	//! Called after every successful update of clientkey_id_map
	std::function<void()> on_update;

	template<typename... Args>
	Grapher(Args&&...) : t(this) {
		t.template start<Grapher, &Grapher::query>(0, 300000);
	}

	void query() {
		auto url = asyncio::resolve_subgraph_url("/subgraphs/name/marlinprotocol/staking-arb1");
		SPDLOG_DEBUG("Grapher: querying {}", asyncio::redact_url(url));
		asyncio::http_post(
			url,
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4\\\"}){clientKey, id}}\"}",
			[this, url](asyncio::HttpResponse&& res) {
				did_query(url, std::move(res));
			}
		);
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

		rapidjson::Document JData;
		JData.Parse(res.body.data(), res.body.size());
		if(JData.HasParseError() || !JData.IsObject() || !JData.HasMember("data") || !JData["data"].IsObject()
			|| !JData["data"].HasMember("clusters") || !JData["data"]["clusters"].IsArray()) {
			SPDLOG_ERROR("Grapher: unexpected subgraph response: {}", res.body.substr(0, 200));
			return;
		}

		auto& clusters = JData["data"]["clusters"];
		std::map<std::string, std::string> m;
		for(rapidjson::SizeType i = 0; i < clusters.Size(); i++){
			auto& cluster = clusters[i];
			if(!cluster.IsObject() || !cluster.HasMember("clientKey") || !cluster["clientKey"].IsString()
				|| !cluster.HasMember("id") || !cluster["id"].IsString()) {
				continue;
			}
			m[cluster["clientKey"].GetString()] = cluster["id"].GetString();
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
