#pragma once

class MyPlugin;
/**
 * @brief Registers all track-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 * 
 * Endpoints registered:
 * - GET  /track/info        - Get current track information (title, artist, album, genre, play count, bitrate, sample rate, rating)
 * - GET  /track/cover       - Get current track cover image
 * - GET  /track/lyrics      - Get track lyrics by playlistId and songIndex
 * - GET  /track/download    - Download track file by playlistId and songIndex
 */
void RegisterTrackRoutes(MyPlugin *plugin, const std::string& prefix);