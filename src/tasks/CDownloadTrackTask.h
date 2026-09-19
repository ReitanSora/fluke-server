#pragma once
#include "../core/Plugin.h"

/**
 * @class CDownloadTrackTask
 * @brief Task that downloads a specific track.
 *
 * This class implements the IAIMPTask interface and is used to query AIMP for
 * downloading a given track.
 *
 * Threading / lifetime notes:
 * - Implements COM-style reference counting via `AddRef`/`Release`.
 * - The task is executed through AIMP's task system; `Execute` runs on the
 *   AIMP thread context supplied by the host (caller).
 * - All COM interfaces obtained inside `Execute` are released before returning.
 */
class CDownloadTrackTask : public IAIMPTask
{
private:
    /**
     * @brief COM reference count (volatile for atomic ops).
     *
     * Initialized to 1 to represent the creator's reference.
     */
    volatile ULONG _refCount = 1;

    /**
     * @brief Non-owning pointer to the main plugin instance.
     *
     * Used to access AIMP services and helper methods. The plugin instance is
     * expected to outlive this task.
     */
    MyPlugin* _plugin;

    /**
     * @brief Playlist ID for which to retrieve the track URI.
     *
     * This UTF-8 string identifies the target playlist within AIMP. It is
     * provided at construction time and used by `Execute` to locate the
     * corresponding playlist. This class does not own external memory for the
     * string; it stores its own copy.
     */
    std::string _playlistId;

    int _songIndex;

    bool _hasErrors = true;

    std::string _filePath;

public:
    /**
     * @brief Constructs the task with a reference to the plugin.
     * @param plugin Pointer to `MyPlugin` that provides access to AIMP services.
     *
     * The constructor does not take ownership of `plugin`.
     */
    CDownloadTrackTask(MyPlugin* plugin, const std::string& _playlistId, const int& _songIndex);

    /**
     * @brief Standard COM QueryInterface implementation.
     * @param riid Interface ID requested.
     * @param ppvObject Receives interface pointer on success.
     * @return S_OK if supported, E_NOINTERFACE or E_POINTER otherwise.
     *
     * On success this method calls `AddRef()` on the returned interface.
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject) override;

    /**
     * @brief Increments the COM reference count.
     * @return The new reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief Decrements the COM reference count and deletes the object at zero.
     * @return The new reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief Executes the task to obtain the current playlist.
     * @param Owner Task owner provided by AIMP (may be nullptr or unused).
     *
     * Execution steps (high level):
     *  - Acquire the player service from the plugin/core.
     *  - Ask the player for the currently playing `IAIMPPlaylistItem`.
     *  - From the item obtain the parent `IAIMPPlaylist`.
     *  - Read playlist properties (id, name) and item count into `_result`.
     *  - Set `_result.found = true` if a playlist was read successfully.
     *
     * All COM interfaces acquired during execution are released before return.
     */
    virtual void WINAPI Execute(IAIMPTaskOwner* Owner) override;

    bool HasErrors() const { return _hasErrors; }

    std::string GetResponse() const { return _filePath; }

};