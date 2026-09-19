#include "pch.h"
#include "../tasks/CCreatePlaylistTask.h"

CCreatePlaylistTask::CCreatePlaylistTask(MyPlugin* plugin, const std::string& _playlistName)
    : _plugin(plugin), _playlistName(_playlistName) {
}

HRESULT WINAPI CCreatePlaylistTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CCreatePlaylistTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CCreatePlaylistTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CCreatePlaylistTask::Execute(IAIMPTaskOwner* Owner)
{
    IAIMPString* name = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistName, &name))) return;
    
    IAIMPPlaylist* newPlaylist = nullptr;

	IAIMPServicePlaylistManager* manager = _plugin->GetPlaylistService();
    
    if(SUCCEEDED(manager->CreatePlaylist(name, FALSE, &newPlaylist)))
    {
		IAIMPPropertyList *props = nullptr;
        if (SUCCEEDED(newPlaylist->QueryInterface(IID_IAIMPPropertyList, (void**)&props)))
        {
            _createdPlaylistId = _plugin->GetPropertyText(props, AIMP_PLAYLIST_PROPID_ID, "");
        }
        newPlaylist->Release();
    }
    name->Release();
}