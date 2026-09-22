#include "pch.h"
#include "PlayerRoutes.h"
#include "../core/Plugin.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;


// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Sends a command to AIMP via WM_COMMAND
 */
static void SendAIMPCommand(DWORD command)
{
    HWND hAIMP = FindWindowA(AIMPRemoteAccessClass, NULL);
    if (hAIMP)
    {
        SendMessage(hAIMP, WM_AIMP_COMMAND, command, 0);
    }
}

/**
 * @brief Gets a property value from AIMP via WM_AIMP_PROPERTY
 */
static LRESULT GetAIMPProperty(DWORD property)
{
    HWND hAIMP = FindWindowA(AIMPRemoteAccessClass, NULL);
    if (hAIMP)
    {
        return SendMessage(hAIMP, WM_AIMP_PROPERTY, property, 0);
    }
    return 0;
}

/**
 * @brief Sets a property value in AIMP via WM_AIMP_PROPERTY
 */
static void SetAIMPProperty(DWORD property, LPARAM value)
{
    HWND hAIMP = FindWindowA(AIMPRemoteAccessClass, NULL);
    if (hAIMP)
    {
        SendMessage(hAIMP, WM_AIMP_PROPERTY, property | AIMP_RA_PROPVALUE_SET, value);
    }
}


// =============================================================================
// Route Handlers
// =============================================================================

static void HandleGetPlayerState(const httplib::Request& req, httplib::Response& res)
{
    LRESULT state = GetAIMPProperty(AIMP_RA_PROPERTY_PLAYER_STATE);
    LRESULT position = GetAIMPProperty(AIMP_RA_PROPERTY_PLAYER_POSITION);
    LRESULT duration = GetAIMPProperty(AIMP_RA_PROPERTY_PLAYER_DURATION);
    LRESULT volume = GetAIMPProperty(AIMP_RA_PROPERTY_VOLUME);
    LRESULT mute = GetAIMPProperty(AIMP_RA_PROPERTY_MUTE);
    LRESULT repeat = GetAIMPProperty(AIMP_RA_PROPERTY_TRACK_REPEAT);
    LRESULT shuffle = GetAIMPProperty(AIMP_RA_PROPERTY_TRACK_SHUFFLE);
    
    json response = {
        {"state", state},          // 0=stopped, 1=paused, 2=playing
        {"position", position},    // Current position in ms
        {"duration", duration},    // Total duration in ms
        {"volume", volume},        // Volume 0-100
        {"mute", mute != 0},
        {"shuffle", shuffle != 0},
        {"repeat", repeat != 0}
    };
    
    res.status = 200;
    res.set_content(response.dump(), "application/json");
}

static void HandleGetVolume(const httplib::Request& req, httplib::Response& res)
{
    LRESULT volume = GetAIMPProperty(AIMP_RA_PROPERTY_VOLUME);
    
    json response = {
        {"volume", volume}
    };
    
    res.status = 200;
    res.set_content(response.dump(), "application/json");
}

static void HandlePlayPause(const httplib::Request& req, httplib::Response& res)
{
    SendAIMPCommand(AIMP_RA_CMD_PLAYPAUSE);
    
    res.status = 204;
}

static void HandleStop(const httplib::Request& req, httplib::Response& res)
 {
    SendAIMPCommand(AIMP_RA_CMD_STOP);
   
    res.status = 204;
}

static void HandleNext(const httplib::Request& req, httplib::Response& res)
{
    SendAIMPCommand(AIMP_RA_CMD_NEXT);
    
    res.status = 204;
}

static void HandlePrevious(const httplib::Request& req, httplib::Response& res)
{
    SendAIMPCommand(AIMP_RA_CMD_PREV);
    
    res.status = 204;
}

static void HandleSetVolume(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        json body = json::parse(req.body);
        
        if (!body.contains("volume"))
        {
            res.status = 422;
            res.set_content(json{{"error", "Missing 'volume' field"}}.dump(), "application/json");
            return;
        }
        
        int volume = body["volume"].get<int>();
        
        if (volume < 0 || volume > 100)
        {
            res.status = 422;
            res.set_content(json{{"error", "Volume must be between 0 and 100"}}.dump(), "application/json");
            return;
        }
        
        SetAIMPProperty(AIMP_RA_PROPERTY_VOLUME, volume);
        
        res.status = 204;
    }
    catch (const json::exception&)
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to set volume"}}.dump(), "application/json");
    }
}

static void HandleSeek(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        json body = json::parse(req.body);
        
        if (!body.contains("position"))
        {
            res.status = 422;
            res.set_content(json{{"error", "Missing 'position' field"}}.dump(), "application/json");
            return;
        }
        
        int position = body["position"].get<int>();
        
        SetAIMPProperty(AIMP_RA_PROPERTY_PLAYER_POSITION, position);
        
        res.status = 204;
    }
    catch (const json::exception&)
    {
        res.status = 500;
        res.set_content(json{{"error", "Failed to seek"}}.dump(), "application/json");
    }
}

static void HandleToggleShuffle(const httplib::Request& req, httplib::Response& res)
{
    LRESULT currentShuffle = GetAIMPProperty(AIMP_RA_PROPERTY_TRACK_SHUFFLE);
    LRESULT newShuffle = (currentShuffle == 0) ? 1 : 0;

    SetAIMPProperty(AIMP_RA_PROPERTY_TRACK_SHUFFLE, newShuffle);

    res.status = 204;
}

static void HandleToggleRepeat(const httplib::Request& req, httplib::Response& res)
{
    LRESULT currentRepeat = GetAIMPProperty(AIMP_RA_PROPERTY_TRACK_REPEAT);
    LRESULT newRepeat = (currentRepeat == 0) ? 1 : 0;

    SetAIMPProperty(AIMP_RA_PROPERTY_TRACK_REPEAT, newRepeat);

    res.status = 204;
}

static void HandleToggleMute(const httplib::Request& req, httplib::Response& res)
{
    LRESULT currentMute = GetAIMPProperty(AIMP_RA_PROPERTY_MUTE);
    LRESULT newMute = (currentMute == 0) ? 1 : 0;

    SetAIMPProperty(AIMP_RA_PROPERTY_MUTE, newMute);

    res.status = 204;
}


// =============================================================================
// Route Registration
// =============================================================================

void RegisterPlayerRoutes(MyPlugin *plugin, const std::string& prefix)
{
    auto& svr = plugin->GetHttpServer();
    
    // GET endpoints
    svr.Get(prefix + "/state", HandleGetPlayerState);
    svr.Get(prefix + "/volume", HandleGetVolume);
    
    // POST endpoints - playback control
    svr.Post(prefix + "/playpause", HandlePlayPause);
    svr.Post(prefix + "/stop", HandleStop);
    svr.Post(prefix + "/next", HandleNext);
    svr.Post(prefix + "/previous", HandlePrevious);
    
    // POST endpoints - settings
    svr.Post(prefix + "/volume", HandleSetVolume);
    svr.Post(prefix + "/seek", HandleSeek);
    svr.Post(prefix + "/mute", HandleToggleMute);
    svr.Post(prefix + "/shuffle", HandleToggleShuffle);
    svr.Post(prefix + "/repeat", HandleToggleRepeat);
}