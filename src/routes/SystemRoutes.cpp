#include "pch.h"
#include "../core/Plugin.h"
#include "SystemRoutes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void RegisterSystemRoutes(MyPlugin* plugin, const std::string& prefix) 
{
	auto &svr = plugin->GetHttpServer();

	svr.Get(prefix + "/health", [plugin](const httplib::Request& req, httplib::Response& res){
		res.status = 204;
		});

	svr.Get(prefix + "/system/info", [plugin](const httplib::Request& req, httplib::Response& res) {
		json response = {
			{"plugin", {
				{"name", "Fluke: AIMP Remote Control"},
				{"version", "2.0.0"},
				{"description", "Remote control plugin for AIMP"}
			}}
		};

		res.status = 200;
		res.set_content(response.dump(), "application/json; charset=utf-8");

		});
}