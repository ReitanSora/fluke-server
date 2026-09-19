#include "pch.h"
#include <fstream>
#include "TrackRoutes.h"
#include "../core/Plugin.h"
#include <nlohmann/json.hpp>
#include "../helpers/AlbumArtHelper.h"
#include "../helpers/TrackInfoHelper.h"
#include "../tasks/CGetTrackInfoTask.h"
#include "../tasks/CGetTrackLyricsTask.h"
#include "../tasks/CDownloadTrackTask.h"

using json = nlohmann::json;

json parseLrcLyrics(const std::string& lrcRaw) {
    json linesArray = json::array();
    std::stringstream ss(lrcRaw);
    std::string line;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        int minutes = 0;
        float seconds = 0.0f;
        char textBuffer[1024] = { 0 };


        int matched = std::sscanf(line.c_str(), "[%d:%f]%1023[^\n]", &minutes, &seconds, textBuffer);

        if (matched >= 2) {
            int totalMilliseconds = static_cast<int>(((minutes * 60) + seconds) * 1000.0f);

            std::string text = (matched == 3) ? std::string(textBuffer) : "";

            size_t start = text.find_first_not_of(" \t");
            if (start != std::string::npos) {
                text = text.substr(start);
            }
            else if (text.find_first_of(" \t") == 0) {
                text = "";
            }

            json lineObj;
            lineObj["time"] = totalMilliseconds;
            lineObj["text"] = text;

            linesArray.push_back(lineObj);
        }
    }

    return linesArray;
}

json parseUnsyncedLyrics(const std::string& rawText) {
    json linesArray = json::array();
    std::stringstream ss(rawText);
    std::string line;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        linesArray.push_back(line);
    }

    return linesArray;
}

// =============================================================================
// Route Handlers
// =============================================================================

static void HandleGetTrackInfo(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!plugin->GetThreadService())
    {
        res.status = 503;
        res.set_content(json{{"error", "Thread service unavailable"}}.dump(), "application/json");
        return;
    }

	CGetTrackInfoTask* task = new CGetTrackInfoTask(plugin);

	HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);
	
    if (SUCCEEDED(hr))
    {
        const json &response = task->GetResult();
        if(response.contains("error"))
        {
            res.status = 404;
            res.set_content(response.dump(), "application/json; charset=utf-8");
        }
        else 
        {
            res.status = 200;
            res.set_content(response.dump(), "application/json; charset=utf-8");
        }
    }
    else
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve track info"}}.dump(), "application/json");
    }

    task->Release();
    
}

static void HandleGetTrackCover(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    IAIMPFileInfo *fileInfo = nullptr;

    if (FAILED(plugin->GetPlayerService()->GetInfo(&fileInfo)))
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve track info"}}.dump(), "application/json; charset=utf-8");
        return;
    }

    IAIMPString *fileURI = nullptr;

    if (FAILED(fileInfo->GetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, IID_IAIMPString, (void **)&fileURI)))
    {
        fileInfo->Release();
        res.status = 500;
        res.set_content(json{{"error", "Failed to retrieve track file URI"}}.dump(), "application/json; charset=utf-8");
        return;
    }

    std::vector<unsigned char> imageBytes;
    TTaskHandle taskID = 0;

    HRESULT hr = plugin->GetAlbumArtService()->Get(fileURI, nullptr, nullptr, AIMP_SERVICE_ALBUMART_FLAGS_ORIGINAL, OnAlbumArtReceive, &imageBytes, &taskID);

    int attemps = 0;
    while (imageBytes.empty() && attemps < 50)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        attemps++;
    }

    fileURI->Release();
    fileInfo->Release();

    if (!imageBytes.empty())
    {
        std::string mimeType = get_mime_type(imageBytes);

        res.status = 200;
        res.set_content(reinterpret_cast<const char *>(imageBytes.data()), imageBytes.size(), mimeType);
    }
    else
    {
        res.status = 404;
        res.set_content(json{{"error", "Album art not found or retrieval timeout"}}.dump(), "application/json; charset=utf-8");
        return;
    }
}

static void HandleGetTrackLyrics(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    std::string playlistId = req.get_param_value("playlistId");
    int songIndex = std::stoi(req.get_param_value("songIndex"));

    CGetTrackLyricsTask* task = new CGetTrackLyricsTask(plugin, playlistId, songIndex);

    plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    std::string lyrics = task->GetLyrics();
    task->Release();

    if (!lyrics.empty()) {
        json responseJson;

        if (lyrics.find('[') != std::string::npos && lyrics.find(']') != std::string::npos) {
            responseJson["type"] = "synced";
            responseJson["lines"] = parseLrcLyrics(lyrics);
        }
        else {
            responseJson["type"] = "unsynced";
            responseJson["text"] = parseUnsyncedLyrics(lyrics);
        }

        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    }
    else {
        res.status = 404;
        res.set_content(json{{"error", "No embedded lyrics found"}}.dump(), "application/json");
    }

}

static void HandleDownloadTrack(MyPlugin* plugin, const httplib::Request& req, httplib::Response& res)
{

    if (!req.has_param("playlistId") || !req.has_param("songIndex"))
    {
        res.status = 422;
        res.set_content(json{ {"error", "Missing playlist id or song index"} }.dump(), "application/json");
        return;
    }

    std::string playlistId = req.get_param_value("playlistId");
    int songIndex = std::stoi(req.get_param_value("songIndex"));

    CDownloadTrackTask* task = new CDownloadTrackTask(plugin, playlistId, songIndex);

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (FAILED(hr) || task->HasErrors())
    {
        res.status = 500;
        res.set_content(json{ {"error", "Failed to download track"} }.dump(), "application/json");
        task->Release();
        return;
    }

    std::string filePath = task->GetResponse();
    task->Release();

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        res.status = 404;
        res.set_content(json{ {"error", "File not found"} }.dump(), "application/json");
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string fileName = filePath;
    size_t pos = filePath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        fileName = filePath.substr(pos + 1);
    }

    res.set_header("Content-Disposition", "attachment; filename=\"" + fileName + "\"");

    res.set_content_provider(
        size,
        "application/octet-stream",
        [filePath](size_t offset, size_t length, httplib::DataSink& sink) {
            std::ifstream is(filePath, std::ios::binary);
            if (!is.is_open()) return false;

            is.seekg(offset, std::ios::beg);

            std::vector<char> buffer(length);
            is.read(buffer.data(), length);

            std::streamsize bytesRead = is.gcount();
            if (bytesRead > 0) {
                sink.write(buffer.data(), static_cast<size_t>(bytesRead));
            }
            return true;
        }
    );
}

// =============================================================================
// Route Registration
// =============================================================================

void RegisterTrackRoutes(MyPlugin* plugin, const std::string& prefix)
{
    auto &svr = plugin->GetHttpServer();

    // GET endpoints
    svr.Get(prefix + "/track/info", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetTrackInfo(plugin, req, res); });

    svr.Get(prefix + "/track/cover", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetTrackCover(plugin, req, res); });

    svr.Get(prefix + "/track/lyrics", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetTrackLyrics(plugin, req, res); });

    svr.Get(prefix + "/track/download", [plugin](const httplib::Request& req, httplib::Response& res)
            { HandleDownloadTrack(plugin, req, res); });
}