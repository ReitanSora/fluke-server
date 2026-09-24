#pragma once

class MyPlugin;
/**
 * @brief Registers all queue-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 *
 * Endpoints registered:
 * - GET	/queue/songs       - Add items to the queue
 * - DELETE /queue/songs       - Delete items from the queue
 */
void RegisterQueueRoutes(MyPlugin* plugin, const std::string& prefix);