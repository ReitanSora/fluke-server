#pragma once

class MyPlugin;
/**
 * @brief Registers all system-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 *
 * Endpoints registered:
 * - GET  /system/health		- Get health status of the plugin
 * - GET  /system/info			- Get plugin information
 */
void RegisterSystemRoutes(MyPlugin* plugin, const std::string& prefix);