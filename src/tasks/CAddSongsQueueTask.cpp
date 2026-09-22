#include "pch.h"
#include "../tasks/CAddSongsQueueTask.h"

CAddSongsQueueTask::CAddSongsQueueTask(MyPlugin* plugin, const std::string& _playlistId, const std::vector<int>& _songsIndexes, const bool& _insertAtBeginning)
    : _plugin(plugin), _playlistId(_playlistId), _songsIndexes(_songsIndexes), _insertAtBeginning(_insertAtBeginning) {
}

HRESULT WINAPI CAddSongsQueueTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CAddSongsQueueTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CAddSongsQueueTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CAddSongsQueueTask::Execute(IAIMPTaskOwner* Owner)
{
    if (_songsIndexes.empty()) return;

	IAIMPServicePlaylistManager* manager = _plugin->GetPlaylistService();
	if (!manager) return;

    IAIMPString* playlistId = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistId, &playlistId))) return;

    IAIMPPlaylist* playlist = nullptr;
    if (FAILED(manager->GetLoadedPlaylistByID(playlistId, &playlist)))
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

    if (_songsIndexes.size() == 1)
    {
        IAIMPPlaylistItem* item = nullptr;
        if (SUCCEEDED(playlist->GetItem(_songsIndexes[0], IID_IAIMPPlaylistItem, (void **)&item)))
        {
            queue->Add(item, _insertAtBeginning);
            _hasErrors = false;
            queue->Release();
            item->Release();
        }
    }
    else
    {
        IAIMPObjectList* list = nullptr;
        if (SUCCEEDED(_plugin->GetCore()->CreateObject(IID_IAIMPObjectList, (void**)&list)))
        {
            for (int index : _songsIndexes)
            {
                IAIMPPlaylistItem* item = nullptr;
                if (SUCCEEDED(playlist->GetItem(index, IID_IAIMPPlaylistItem, (void**)&item)))
                {
                    list->Add(item);
                    item->Release();
                }
            }

            queue->AddList(list, _insertAtBeginning);
            _hasErrors = false;
            list->Release();
            queue->Release();
        }
    }

    playlistId->Release();
    playlist->Release();

}