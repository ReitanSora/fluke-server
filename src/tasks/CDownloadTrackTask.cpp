#include "pch.h"
#include "CDownloadTrackTask.h"

CDownloadTrackTask::CDownloadTrackTask(MyPlugin* plugin, const std::string& _playlistId, const int& _songIndex)
    : _plugin(plugin), _playlistId(_playlistId), _songIndex(_songIndex) {
}

HRESULT WINAPI CDownloadTrackTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CDownloadTrackTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CDownloadTrackTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CDownloadTrackTask::Execute(IAIMPTaskOwner* Owner)
{
	IAIMPString* playlistId = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistId, &playlistId))) return;

    IAIMPPlaylist* playlist = nullptr;
    if (FAILED(_plugin->GetPlaylistService()->GetLoadedPlaylistByID(playlistId, &playlist)))
    {
        playlistId->Release();
        return;
    }

    IAIMPPlaylistItem* item = nullptr;
    if (SUCCEEDED(playlist->GetItem(_songIndex, IID_IAIMPPlaylistItem, (void**)&item)))
    {
        _filePath = _plugin->GetPropertyText(item, AIMP_PLAYLISTITEM_PROPID_FILENAME, "");
		_hasErrors = false;
        item->Release();
    }

	playlistId->Release();
	playlist->Release();
        
}