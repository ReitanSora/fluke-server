#pragma once

class MyPlugin;
/**
 * @brief Registers all player-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 * 
 * Endpoints registered:
 * - GET  /player/state        - Get current player state (playing/paused/stopped)
 * - GET  /player/volume       - Get current volume (0-100)
 * - POST /player/playpause    - Toggle playback
 * - POST /player/stop         - Stop playback
 * - POST /player/next         - Skip to next track
 * - POST /player/previous     - Go to previous track
 * - POST /player/volume       - Set volume (body: {"volume": 0-100})
 * - POST /player/seek         - Seek to position (body: {"position": seconds})
 * - POST /player/mute         - Toggle mute
 * - POST /player/shuffle      - Toggle shuffle
 * - POST /player/repeat       - Toggle repeat
 */
void RegisterPlayerRoutes(MyPlugin *plugin, const std::string& prefix);