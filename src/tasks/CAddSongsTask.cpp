#include "pch.h"
#include "../tasks/CAddSongsTask.h"

CAddSongsTask::CAddSongsTask(MyPlugin* plugin, const std::string& _targetPlaylistId, const std::string& _sourcePlaylistId,const std::vector<int>& _songsIndexes)
    : _plugin(plugin), _targetPlaylistId(_targetPlaylistId), _sourcePlaylistId(_sourcePlaylistId), _songsIndexes(_songsIndexes) {
}

HRESULT WINAPI CAddSongsTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CAddSongsTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CAddSongsTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CAddSongsTask::Execute(IAIMPTaskOwner* Owner)
{
    if (_songsIndexes.empty()) return;

    IAIMPString* targetId = nullptr;
    IAIMPString* sourceId = nullptr;

    _plugin->CreateAIMPString(_targetPlaylistId, &targetId);
    _plugin->CreateAIMPString(_sourcePlaylistId, &sourceId);

    IAIMPPlaylist* targetPlaylist = nullptr;
    IAIMPPlaylist* sourcePlaylist = nullptr;

    if (SUCCEEDED(_plugin->GetPlaylistService()->GetLoadedPlaylistByID(targetId, &targetPlaylist)) &&
        SUCCEEDED(_plugin->GetPlaylistService()->GetLoadedPlaylistByID(sourceId, &sourcePlaylist)))
    {
        _playlistsFound = true;
        int insertIndex = targetPlaylist->GetItemCount();

        if (_songsIndexes.size() == 1)
        {
            IAIMPPlaylistItem* item = nullptr;
            if (SUCCEEDED(sourcePlaylist->GetItem(_songsIndexes[0], IID_IAIMPPlaylistItem, (void**)&item)))
            {
                IAIMPString* fileUri = nullptr;
                if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILENAME, IID_IAIMPString, (void**)&fileUri)))
                {
                    targetPlaylist->Add(fileUri, 0, insertIndex);
                    fileUri->Release();
                }
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
                    if (SUCCEEDED(sourcePlaylist->GetItem(index, IID_IAIMPPlaylistItem, (void**) & item)))
                    {
                        IAIMPString* fileUri = nullptr;
                        if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILENAME, IID_IAIMPString, (void**)&fileUri)))
                        {
                            list->Add(fileUri);
                            fileUri->Release();
                        }
                        item->Release();
                    }
                }
                targetPlaylist->AddList(list, 0, insertIndex);
                list->Release();
            }
        }
        targetPlaylist->Release();
        sourcePlaylist->Release();
    }
    targetId->Release();
    sourceId->Release(); 
}