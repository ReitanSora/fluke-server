#include "pch.h"
#include <nlohmann/json.hpp>
#include "PlaylistRoutes.h"
#include "../core/Plugin.h"
#include "../tasks/CGetPlaylistsTask.h"
#include "../tasks/CGetPlaylistInfoTask.h"
#include "../tasks/CGetPlaylistItemsTask.h"
#include "../tasks/CGetPlaylistStatsTask.h"
#include "../tasks/CGetCurrentPlaylistTask.h"
#include "../tasks/CPlayItemTask.h"
#include "../tasks/CGetPlaylistTrackUriTask.h"
#include "../tasks/CDeletePlaylistItemTask.h"
#include "../tasks/CCreatePlaylistTask.h"
#include "../tasks/CAddSongsTask.h"
#include "../helpers/AlbumArtHelper.h"

using json = nlohmann::json;

// =============================================================================
// Route Handlers
// =============================================================================

static void HandleGetPlaylistList(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{

    if (!plugin->GetPlaylistService() || !plugin->GetThreadService())
    {
        res.status = 503;
        res.set_content(json{{"error", "Services unavailable"}}.dump(), "application/json");
        return;
    }

    CGetPlaylistsTask *task = new CGetPlaylistsTask(plugin);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        json response = json::array();

        const auto &results = task->GetResults();

        for (const auto &pl : results)
        {
            response.push_back({{"playlistId", pl.id},
                                {"name", pl.name},
                                {"itemCount", pl.itemCount}});
        }

        res.status = 200;
        res.set_content(response.dump(), "application/json; charset=utf-8");
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve playlists"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandleGetCurrentPlaylist(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!plugin->GetPlaylistService() || !plugin->GetThreadService())
    {
        res.status = 503;
        res.set_content(json{{"error", "Services unavailable"}}.dump(), "application/json");
        return;
    }

    CGetCurrentPlaylistTask *task = new CGetCurrentPlaylistTask(plugin);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &result = task->GetResult();
        if (result.found)
        {
            json response = {
                {"playlistId", result.id},
                {"name", result.name},
                {"itemCount", result.itemCount}
            };

            res.status = 200;
            res.set_content(response.dump(), "application/json; charset=utf-8");
        }
        else
        {
            res.status = 404;
            res.set_content(json{{"error", "Current playlist not found"}}.dump(), "application/json");
        }
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to get current playlist"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistInfo(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!req.has_param("playlistId"))
    {
        res.status = 422;
        res.set_content(json{{"error", "Missing playlist id"}}.dump(), "application/json");
        return;
    }
    
    std::string playlistId = req.get_param_value("playlistId");

    CGetPlaylistInfoTask *task = new CGetPlaylistInfoTask(plugin, playlistId);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &r = task->GetResult();
        if (r.found)
        {
            json response = {
                {"playlistId", r.id},
                {"name", r.name},
                {"itemCount", r.itemCount},
                {"duration", r.duration},
                {"playingIndex", r.playingIndex},
                {"isReadOnly", r.isReadOnly} 
            };

            res.status = 200;
            res.set_content(response.dump(), "application/json; charset=utf-8");
        }
        else
        {
            res.status = 404;
            res.set_content(json{{"error", "Playlist not found"}}.dump(), "application/json");
        }
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve playlist info"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistStats(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!req.has_param("playlistId"))
    {
        res.status = 422;
        res.set_content(json{{"error", "Missing playlist id"}}.dump(), "application/json");
        return;
    }

    std::string playlistId = req.get_param_value("playlistId");

    CGetPlaylistStatsTask *task = new CGetPlaylistStatsTask(plugin, playlistId);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &r = task->GetResult();
        if (r.found)
        {
            json response = {
                {"genres", r.genres},
                {"artists", r.artists},
                {"artistCount", r.artistCount},
                {"albumCount", r.albumCount},
                {"avgBitrate", r.avgBitrate},
                {"avgRating", r.avgRating},
                {"tracksWithRating", r.tracksWithRating},
                {"totalPlayCount", r.totalPlayCount},
                {"tracksNeverPlayed", r.tracksNeverPlayed},
                {"totalSizeBytes", r.totalSizeBytes} 
            };

            res.status = 200;
            res.set_content(response.dump(), "application/json; charset=utf-8");
        }
        else
        {
            res.status = 404;
            res.set_content(json{{"error", "Playlist not found"}}.dump(), "application/json");
        }
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve playlist stats"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistItems(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!req.has_param("playlistId"))
    {
        res.status = 422;
        res.set_content(json{{"error", "Missing Playlist ID"}}.dump(), "application/json");
        return;
    }

    std::string playlistId = req.get_param_value("playlistId");

    CGetPlaylistItemsTask *task = new CGetPlaylistItemsTask(plugin, playlistId);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        json response = json::array();
        const auto &songs = task->GetResults();

        for (const auto &s : songs)
        {
            response.push_back({{"songIndex", s.index},
                                {"title", s.title},
                                {"artist", s.artist},
                                {"album", s.album},
                                {"bitrate", s.bitrate},
                                {"sampleRate", s.sampleRate},
                                {"duration", s.duration}});
        }
        res.status = 200;
        res.set_content(response.dump(), "application/json; charset=utf-8");
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve playlist items"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandlePlayPlaylistItem(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!req.has_param("playlistId") || !req.has_param("songIndex"))
    {
        res.status = 422;
        res.set_content(json{{"error", "Missing Playlist ID or Song index"}}.dump(), "application/json");
        return;
    }

    std::string playlistId = req.get_param_value("playlistId");
    int songIndex = std::stoi(req.get_param_value("songIndex"));

    CPlayItemTask *task = new CPlayItemTask(plugin, playlistId, songIndex);

    if (SUCCEEDED(plugin->GetThreadService()->ExecuteInMainThread(task, 0)))
    {
        res.status = 204;
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to play playlist item"}}.dump(), "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistCover(MyPlugin* plugin, const httplib::Request& req, httplib::Response& res)
{
    if (!req.has_param("playlistId"))
    {
        res.status = 422;
        res.set_content(json{{"error", "Missing playlist id"}}.dump(), "application/json");
        return;
    }

    std::string playlistId = req.get_param_value("playlistId");

    CGetPlaylistTrackUriTask* task = new CGetPlaylistTrackUriTask(plugin, playlistId);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    std::vector<std::string> uris;
    if (SUCCEEDED(hr))
    {
        uris = task->GetFileUris();
    }
    task->Release();

    if (uris.empty())
    {
        res.status = 409;
        res.set_content(json{{"error", "Playlist empty"}}.dump(), "application/json");
        return;
    }

    std::vector<unsigned char> imageBytes;

    for (const auto& uriStr : uris)
    {
        IAIMPString* fileURI = nullptr;
        if (FAILED(plugin->CreateAIMPString(uriStr, &fileURI))) continue;

        TTaskHandle taskID = 0;
        plugin->GetAlbumArtService()->Get(
            fileURI, nullptr, nullptr,
            AIMP_SERVICE_ALBUMART_FLAGS_ORIGINAL,
            OnAlbumArtReceive, &imageBytes, &taskID
        );

        int attempts = 0;
        while (imageBytes.empty() && attempts < 50)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            attempts++;
        }

        fileURI->Release();

        if (!imageBytes.empty())
        {
            break;
        }
    }

    if (!imageBytes.empty())
    {
        std::string mimeType = get_mime_type(imageBytes);
        res.status = 200;
        res.set_content(reinterpret_cast<const char*>(imageBytes.data()), imageBytes.size(), mimeType);
    }
    else
    {
        res.status = 404;
        res.set_content(json{{"error", "Cover art not found in playlist tracks"}}.dump(), "application/json");
    }

}

static void HandleCreatePlaylist(MyPlugin* plugin, const httplib::Request& req, httplib::Response& res)
{
    try
    {
        json body = json::parse(req.body);
        std::string playlistName = body.at("playlistName").get<std::string>();

        CCreatePlaylistTask* task = new CCreatePlaylistTask(plugin, playlistName);
        HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

        if (SUCCEEDED(hr))
        {
            res.status = 201;
        }
        else
        {
            res.status = 500;
            res.set_content(json{ {"error", "Failed to create playlist"} }.dump(), "application/json");
        }
        task->Release();
    }
    catch (const json::exception&)
    {
        res.status = 422;
        res.set_content(json{ {"error", "Invalid playlist name"} }.dump(), "application/json");
    }
}

static void HandleAddSongsToPlaylist(MyPlugin* plugin, const httplib::Request& req, httplib::Response& res)
{
    try
    {
        json body = json::parse(req.body);
        std::string targetPlaylistId = body.at("targetPlaylistId").get<std::string>();
        std::string sourcePlaylistId = body.at("sourcePlaylistId").get<std::string>();
        std::vector<int> songsIndexes = body.at("songsIndexes").get<std::vector<int>>();

		CAddSongsTask* task = new CAddSongsTask(plugin, targetPlaylistId, sourcePlaylistId, songsIndexes);
		HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

        if(SUCCEEDED(hr) && task->HasFoundPlaylists())
        {
            res.status = 204;
        }
        else
        {
            res.status = 500;
            res.set_content(json{ {"error", "Failed to add songs to playlist"} }.dump(), "application/json");
        }
        task->Release();

    }
    catch (const json::exception&)
    {
        res.status = 422;
        res.set_content(json{ {"error", "Invalid JSON body"} }.dump(), "application/json");
    }
}

static void HandleDeletePlaylistItem(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!plugin->GetPlaylistService() || !plugin->GetThreadService())
    {
        res.status = 503;
        res.set_content(json{{"error", "Services unavailable"}}.dump(), "application/json");
        return;
    }

    try
    {
        json body = json::parse(req.body);
        std::string playlistId = body.at("playlistId").get<std::string>();
        std::vector<int> songsIndexes = body.at("songsIndexes").get<std::vector<int>>();
        boolean physicalDelete = body.at("physicalDelete").get<boolean>();

        CDeletePlaylistItemTask* task = new CDeletePlaylistItemTask(plugin, playlistId, songsIndexes, physicalDelete);

        HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

        if (SUCCEEDED(hr))
        {
            if (task->HasErrors())
            {
                res.status = 422;
                res.set_content(json{{"error", "One or more item indexes are out of bounds or invalid."}}.dump(), "application/json");
            }
            else
            {
                res.status = 204;
            }
        }
        else
        {
            res.status = 500;
            res.set_content(json{{"error", "Failed to delete playlist item"}}.dump(), "application/json");
        }

        task->Release();
    }
    catch (const json::exception)
    {
        res.status = 400;
        res.set_content(json{{"error", "Error parsing JSON"}}.dump(), "application/json");
    }
}

// =============================================================================
// Route Registration
// =============================================================================

void RegisterPlaylistRoutes(MyPlugin* plugin, const std::string& prefix)
{
    auto &svr = plugin->GetHttpServer();

    // GET endpoints
    svr.Get(prefix, [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistList(plugin, req, res); });

    svr.Get(prefix + "/current", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetCurrentPlaylist(plugin, req, res); });

    svr.Get(prefix + "/info", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistInfo(plugin, req, res); });

    svr.Get(prefix + "/stats", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistStats(plugin, req, res); });

    svr.Get(prefix + "/items", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistItems(plugin, req, res); });

    svr.Get(prefix + "/play", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandlePlayPlaylistItem(plugin, req, res); });

    svr.Get(prefix + "/cover", [plugin](const httplib::Request& req, httplib::Response& res)
            { HandleGetPlaylistCover(plugin, req, res); });

    // POST endpoints
    svr.Post(prefix, [plugin](const httplib::Request &req, httplib::Response& res)
        {HandleCreatePlaylist(plugin, req, res);});

    svr.Post(prefix + "/songs", [plugin](const httplib::Request& req, httplib::Response& res)
        { HandleAddSongsToPlaylist(plugin, req, res); });

	// DELETE endpoint
    svr.Delete(prefix + "/songs", [plugin](const httplib::Request& req, httplib::Response& res)
            { HandleDeletePlaylistItem(plugin, req, res); });
}