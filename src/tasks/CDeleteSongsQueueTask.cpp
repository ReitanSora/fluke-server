#include "pch.h"
#include "../tasks/CDeleteSongsQueueTask.h"

CDeleteSongsQueueTask::CDeleteSongsQueueTask(MyPlugin* plugin, const std::string& _playlistId, const std::vector<int>& _songsIndexes)
    : _plugin(plugin), _playlistId(_playlistId), _songsIndexes(_songsIndexes) {
}

HRESULT WINAPI CDeleteSongsQueueTask::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAIMPTask))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI CDeleteSongsQueueTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CDeleteSongsQueueTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CDeleteSongsQueueTask::Execute(IAIMPTaskOwner* Owner)
{
    IAIMPString* playlistId = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistId, &playlistId))) return;

    IAIMPPlaylist* playlist = nullptr;
    if (FAILED(_plugin->GetPlaylistService()->GetLoadedPlaylistByID(playlistId, &playlist)))
    {
        playlistId->Release();
        return;
    }

    IAIMPPlaylistQueue* queue = nullptr;
    if (FAILED(_plugin->GetCore()->QueryInterface(IID_IAIMPPlaylistQueue, (void**)&queue)))
    {
        playlistId->Release();
        playlist->Release();
        return;
    };

    for (int index : _songsIndexes)
    {
        IAIMPPlaylistItem* item = nullptr;
        if (SUCCEEDED(playlist->GetItem(index, IID_IAIMPPlaylistItem, (void**) & item)))
        {
			queue->Delete(item);
            _hasErrors = false;
			item->Release();
        }
    }

    queue->Release();
    playlistId->Release();
    playlist->Release();

}