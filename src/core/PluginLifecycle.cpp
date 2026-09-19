#include "pch.h"
#include "Plugin.h"
#include "Config.h"
#include "../routes/PlayerRoutes.h"
#include "../routes/PlaylistRoutes.h"
#include "../routes/TrackRoutes.h"
#include "../routes/SystemRoutes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief Initialize the plugin and acquire required AIMP services.
 *
 * This function is called by AIMP to initialize the plugin. It performs
 * the following responsibilities:
 * - Validates the provided `IAIMPCore*` pointer and takes ownership by
 *   calling `AddRef()` on `_core`.
 * - Queries and stores pointers to the AIMP services used by the plugin:
 *   `IID_IAIMPServicePlayer`, `IID_IAIMPServiceAlbumArt`,
 *   `IID_IAIMPServicePlaylistManager`, and `IID_IAIMPServiceThreads`.
 *   If any required service query fails the function returns `E_FAIL`.
 * - Attempts to hook into AIMP's message dispatcher (`IID_IAIMPServiceMessageDispatcher`)
 *   — if available — by calling `Hook(this)`. The dispatcher pointer is released
 *   after hooking.
 * - Creates and configures the WebSocket server (`_wsServer`) using the ixwebsocket
 *   wrapper:
 *   - Instantiates `_wsServer` with `Config::WEBSOCKET_PORT` and
 *     `Config::WEBSOCKET_HOST`.
 *   - Sets an on-connection callback that:
 *     - Adds a connected client to the `_wsClients` list on `Open`.
 *     - Removes the client from `_wsClients` on `Close` or `Error`.
 *     - Uses `_wsMutex` to synchronize access to `_wsClients`.
 *   - Calls `listen()` and starts the server if listening succeeded.
 * - Registers HTTP API routes by calling `RegisterPlayerRoutes(this)`,
 *   `RegisterPlaylistRoutes(this)` and `RegisterTrackRoutes(this)`.
 * - Launches the HTTP server thread (`_httpThread`) which:
 *   - Sets `_httpRunning = true`, calls `_httpServer.listen()` with
 *     `Config::HTTP_HOST`/`Config::HTTP_PORT`, and clears `_httpRunning`
 *     when `listen()` returns. The thread is owned by `_httpThread`.
 *
 * Threading & COM notes:
 * - Many AIMP COM interfaces are apartment-threaded. Callers and this
 *   initialization code must observe AIMP/COM threading rules. The function
 *   only queries services and sets up networking; any future use of COM
 *   interfaces must be done on the appropriate thread or via AIMP's thread
 *   service (`_threadService`) if required.
 * - `_core->AddRef()` is called to keep the `IAIMPCore` reference valid for
 *   the plugin lifetime. Corresponding `Release()` must be done in `Finalize()`.
 *
 * Resource ownership and cleanup expectations:
 * - On success the plugin stores pointers in member variables:
 *   `_core`, `_playerService`, `_albumArtService`, `_playlistService`,
 *   `_threadService`, `_wsServer`, `_httpThread`.
 * - The WebSocket server (`_wsServer`) is allocated with `new` and must be
 *   stopped and deleted in `Finalize()`.
 * - The HTTP server runs in `_httpThread` and `_httpRunning` indicates whether
 *   it is active; the thread must be joined and server stopped during finalization.
 *
 * Return values:
 * - `S_OK` on successful initialization.
 * - `E_INVALIDARG` if `Core` is null.
 * - `E_FAIL` if any required service query fails or object creation fails.
 *
 * Example usage:
 * - AIMP calls the plugin entry point which forwards `IAIMPCore*` to this
 *   function. After `S_OK` is returned the plugin is ready to accept
 *   websocket and HTTP clients and respond to route requests.
 *
 * See also:
 * - `Finalize()` for the corresponding cleanup logic.
 * - `RegisterPlayerRoutes`, `RegisterPlaylistRoutes`, `RegisterTrackRoutes`
 *   for the HTTP route handlers registered here.
 */
HRESULT WINAPI MyPlugin::Initialize(IAIMPCore* Core)
{

    if (!Core)
        return E_INVALIDARG;
    _core = Core;
    _core->AddRef();

    if (FAILED(_core->QueryInterface(IID_IAIMPServicePlayer, (void**)&_playerService)))
    {
        Finalize();
        return E_FAIL;
    }
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceAlbumArt, (void**)&_albumArtService)))
    {
        Finalize();
        return E_FAIL;
    }
    if (FAILED(_core->QueryInterface(IID_IAIMPServicePlaylistManager, (void**)&_playlistService)))
    {
        Finalize();
        return E_FAIL;
    }
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceThreads, (void**)&_threadService)))
    {
        Finalize();
        return E_FAIL;
    }
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceOptionsDialog, (void**)&_optionsService)))
    {
        Finalize();
        return E_FAIL;
    }

    if (Core != nullptr)
    {
        Core->RegisterExtension(IID_IAIMPServiceOptionsDialog, static_cast<IAIMPOptionsDialogFrame*>(this));
    }

    IAIMPServiceMessageDispatcher* dispatcher = nullptr;
    if (SUCCEEDED(_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, (void**)&dispatcher)))
    {
        dispatcher->Hook(this);
        dispatcher->Release();
    }

    _wsServer = std::make_unique<ix::WebSocketServer>(Config::WEBSOCKET_PORT, Config::WEBSOCKET_HOST);

    ix::WebSocketPerMessageDeflateOptions defalteOptions(false);
    _wsServer->disablePerMessageDeflate();

    _wsServer->setOnConnectionCallback(
        [this](std::weak_ptr<ix::WebSocket> ws_weak, std::shared_ptr<ix::ConnectionState> state)
        {
            auto ws = ws_weak.lock();
            if (!ws)
                return;

            {
                std::lock_guard<std::mutex> lock(_wsMutex);
                _wsClients.push_back(ws);
            }

            ws->setOnMessageCallback(
                [this, ws_weak](const ix::WebSocketMessagePtr& msg)
                {
                    auto ws = ws_weak.lock();
                    if (!ws)
                        return;

                    if (msg->type == ix::WebSocketMessageType::Close ||
                        msg->type == ix::WebSocketMessageType::Error)
                    {
                        std::lock_guard<std::mutex> lock(_wsMutex);
                        _wsClients.erase(
                            std::remove_if(_wsClients.begin(), _wsClients.end(),
                                [&ws](const std::shared_ptr<ix::WebSocket>& c)
                                {
                                    return c == ws;
                                }),
                            _wsClients.end());
                    }
                });
        });

    auto wsResult = _wsServer->listen();
    if (wsResult.first)
    {
        _wsServer->start();
    }

    _httpServer = std::make_unique<httplib::Server>();

	const std::string prefix = "/api/v2";

    RegisterPlayerRoutes(this, prefix);
    RegisterPlaylistRoutes(this, prefix);
    RegisterTrackRoutes(this, prefix);
    RegisterSystemRoutes(this, prefix);

    _httpThread = std::make_unique<std::thread>([this]()
    {
        _httpServer->listen(Config::HTTP_HOST, Config::HTTP_PORT);
    });

    return S_OK;
}

/**
 * @brief Finalize
 *
 * Perform orderly shutdown and release of resources acquired by the plugin.
 *
 * Responsibilities:
 * - Stop the HTTP server and join the HTTP worker thread if it is running.
 *   - Calls `_httpServer.stop()` to request the server to exit its `listen()` loop.
 *   - If the HTTP thread exists and is joinable, waits for it to finish via `join()`.
 * - Stop and destroy the WebSocket server (`_wsServer`) if it was created.
 *   - Calls `_wsServer->stop()` to terminate listeners/accept loop, then deletes
 *     the heap-allocated server and nulls the pointer.
 * - Clear the list of connected WebSocket clients (`_wsClients`) under the
 *   protection of `_wsMutex` to avoid races with connection callbacks.
 *   - Each client is held by `std::shared_ptr`; clearing the vector allows
 *     those shared pointers to be released so client state can be destroyed.
 * - Unhook from AIMP's `IAIMPServiceMessageDispatcher` (if available).
 *   - Queries the dispatcher via `_core->QueryInterface(...)`. If found, calls
 *     `Unhook(this)` and `Release()` on the dispatcher interface.
 * - Release all acquired AIMP COM service interfaces and the stored `_core`.
 *   - Releases `_playerService`, `_albumArtService`, `_playlistService` and
 *     finally `_core`, nulling each pointer after `Release()` to avoid
 *     dangling references.
 *
 * Threading & COM notes:
 * - This function assumes it is safe to call `stop()` on network servers and
 *   to `join()` the HTTP thread from the current thread. The code uses
 *   `_httpRunning` as an indication that the HTTP server is active before
 *   attempting to stop/join it.
 * - Many AIMP COM interfaces are apartment-threaded. `Finalize()` should be
 *   invoked on the same COM apartment (typically AIMP's main thread) that is
 *   compatible with releasing the interfaces acquired in `Initialize()`.
 *   Releasing COM objects from the wrong thread/apartment can cause undefined
 *   behaviour. The plugin's `Initialize()` documentation explains COM/thread
 *   expectations in more detail.
 *
 * Resource ownership & ordering:
 * - Stop HTTP first so the listening loop returns and the thread can exit.
 * - Stop WebSocket server next and delete the object to free its resources.
 * - Clear client list while holding `_wsMutex` to synchronize with connection
 *   callbacks set up during `Initialize()`.
 * - Unhook from the message dispatcher before releasing `_core` to ensure the
 *   dispatcher will not attempt callbacks into a partially destroyed plugin.
 * - Release service pointers and `_core` last.
 *
 * Return:
 * - Always returns `S_OK`. The function is designed to best-effort stop and
 *   release resources; individual failures are not propagated via the HRESULT.
 */
HRESULT WINAPI MyPlugin::Finalize()
{
    if (_httpServer)
    {
        _httpServer->stop();
    }
    if (_httpThread && _httpThread->joinable())
    {
        _httpThread->join();
    }
    if (_wsServer)
    {
        _wsServer->stop();
        _wsServer.reset();
    }

    {
        std::lock_guard<std::mutex> lock(_wsMutex);
        _wsClients.clear();
    }

    IAIMPServiceMessageDispatcher* dispatcher = nullptr;
    if (_core && SUCCEEDED(_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, (void**)&dispatcher)))
    {
        dispatcher->Unhook(this);
        dispatcher->Release();
    }

    if (_playerService)
    {
        _playerService->Release();
        _playerService = nullptr;
    }
    if (_albumArtService)
    {
        _albumArtService->Release();
        _albumArtService = nullptr;
    }
    if (_playlistService)
    {
        _playlistService->Release();
        _playlistService = nullptr;
    }
    if (_threadService)
    {
        _threadService->Release();
        _threadService = nullptr;
    }

    if (_optionsService)
    {
        _optionsService->Release();
        _optionsService = nullptr;
    }
    if (_core)
    {
        _core->Release();
        _core = nullptr;
    }

    return S_OK;
}

void WINAPI MyPlugin::SystemNotification(int NotifyID, IUnknown* Data) {}

/**
 * BroadcastWS
 *
 * Serialize a JSON object and broadcast the resulting text payload to all
 * connected WebSocket clients stored in `_wsClients`.
 *
 * Behavior:
 * - Serializes `msg` using `msg.dump()` (nlohmann::json).
 * - Acquires `_wsMutex` to protect concurrent access to `_wsClients`.
 * - Removes any clients whose socket `getReadyState()` is not `Open`.
 * - Iterates the remaining clients and calls `send(text)` on each.
 *
 * Thread-safety:
 * - Access to `_wsClients` is synchronized with `_wsMutex` for the duration
 *   of the cleanup + send loop. This prevents race conditions with other
 *   threads that may add/remove clients.
 *
 * Notes / Considerations:
 * - `ix::WebSocket::send` semantics depend on the ixwebsocket library
 *   configuration (it may perform internal queueing or block). If sending
 *   large messages or if client send may block, consider sending
 *   asynchronously or offloading to a worker thread to avoid blocking the
 *   caller.
 * - If `send` can throw, wrap calls in try/catch to avoid terminating the
 *   broadcasting loop.
 *
 * Parameters:
 * - msg: const json& - message to broadcast (expects nlohmann::json).
 */
void MyPlugin::BroadcastWS(const json& msg)
{
    std::string text = msg.dump();
    std::lock_guard<std::mutex> lock(_wsMutex);

    // Remove non-open clients
    _wsClients.erase(
        std::remove_if(_wsClients.begin(), _wsClients.end(),
            [](const std::shared_ptr<ix::WebSocket>& c)
            {
                return c->getReadyState() != ix::ReadyState::Open;
            }),
        _wsClients.end());

    // Send to remaining connected clients
    for (auto& client : _wsClients)
    {
        client->sendText(text);
    }
}