#include "pch.h"
#include <nlohmann/json.hpp>
#include "QueueRoutes.h"
#include "../core/Plugin.h"
#include "../tasks/CAddSongsQueueTask.h"

using json = nlohmann::json;

static void HandleAddSongsToQueue(MyPlugin* plugin, const httplib::Request& req, httplib::Response& res)
{
    try
    {
        json body = json::parse(req.body);
        std::string playlistId = body.at("playlistId").get<std::string>();
        std::vector<int> songsIndexes = body.at("songsIndexes").get<std::vector<int>>();
        bool insertAtBeginning = body.at("insertAtBeginning").get<bool>();

        CAddSongsQueueTask* task = new CAddSongsQueueTask(plugin, playlistId, songsIndexes, insertAtBeginning);

        HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

        if (SUCCEEDED(hr))
        {
            if (task->HasErrors())
            {
                res.status = 500;
                res.set_content(json{ {"error", "Failed to add songs to queue"} }.dump(), "application/json");
            }
            else
            {
                res.status = 204;
            }
        }
        else
        {
            res.status = 500;
            res.set_content(json{ {"error", "Failed to add songs to queue"} }.dump(), "application/json");
        }

        task->Release();

    }
    catch (const json::exception)
    {
        res.status = 422;
        res.set_content(json{ {"error", "Invalid data in JSON"} }.dump(), "application/json");
    }
}

void RegisterQueueRoutes(MyPlugin* plugin, const std::string& prefix)
{
    auto& svr = plugin->GetHttpServer();

    // GET ENDPOINTS
    svr.Get(prefix, [plugin](const httplib::Request& req, httplib::Response& res)
        {});

    // POST ENDPOINTS
    svr.Post(prefix + "/songs", [plugin](const httplib::Request& req, httplib::Response& res)
        {HandleAddSongsToQueue(plugin, req, res); });

    svr.Post(prefix + "/songs/move", [plugin](const httplib::Request& req, httplib::Response& res)
        {});

	// DELETE ENDPOINTS
    svr.Delete(prefix + "/songs", [plugin](const httplib::Request& req, httplib::Response& res)
        {});

}