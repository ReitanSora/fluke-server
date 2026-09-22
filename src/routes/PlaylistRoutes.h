#pragma once

class MyPlugin;
/**
 * @brief Registers all playlist-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 *
 * Endpoints registered:
 * - GET  /playlists		    - Get list of playlists
 * - GET  /playlists/current    - Get current playlist
 * - GET  /playlists/info       - Get info about a playlist
 * - GET  /playlists/stats      - Get stats about a playlist
 * - GET  /playlists/items      - Get all items of a playlist
 * - GET  /playlists/play       - Play a selected item of a playlist
 * - GET  /playlists/cover      - Get cover art for a playlist
 * - POST /playlists            - Create a new playlist
 * - POST /playlists/songs      - Add songs to a playlist
 * - DELETE /playlists/songs    - Delete a playlist item
 */
void RegisterPlaylistRoutes(MyPlugin *plugin, const std::string& prefix);