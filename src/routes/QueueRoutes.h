#pragma once

class MyPlugin;
/**
 * @brief Registers all queue-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 *
 * Endpoints registered:
 * - GET  /queue/add          - Add items to the queue
 */
void RegisterQueueRoutes(MyPlugin* plugin, const std::string& prefix);