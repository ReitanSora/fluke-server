#include "pch.h"
#include "../tasks/CGetQueueTask.h"

CGetQueueTask::CGetQueueTask(MyPlugin* plugin)
    : _plugin(plugin) {
}

HRESULT WINAPI CGetQueueTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CGetQueueTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CGetQueueTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetQueueTask::Execute(IAIMPTaskOwner* Owner)
{
    IAIMPPlaylistQueue* queue = nullptr;
    if (FAILED(_plugin->GetCore()->QueryInterface(IID_IAIMPPlaylistQueue, (void**)&queue))) return;

    int itemCount = queue->GetItemCount();

    for (int i = 0; i < itemCount; i++)
    {
        IAIMPPlaylistItem* item = nullptr;
        if (SUCCEEDED(queue->GetItem(i, IID_IAIMPPlaylistItem, (void**)&item)))
        {
            IAIMPFileInfo* fileInfo = nullptr;
            if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO, IID_IAIMPFileInfo, (void**)&fileInfo)))
            {
                std::string title = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_TITLE, "Untitled");
                std::string artist = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ARTIST, "Unknown Artist");

                double duration = 0.0;

                fileInfo->GetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, &duration);

                std::string playlistId = "";
                int songIndex = 0;

                IAIMPPlaylist* playlist = nullptr;
                if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_PLAYLIST, IID_IAIMPPlaylist, (void**)&playlist)))
                {
                    IAIMPPropertyList* propList = nullptr;
                    if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void**)&propList)))
                    {
                        playlistId = _plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_ID, "Unknown");
                        propList->Release();
                    }
                    playlist->Release();
                }

                int tempIndex = 0;
                if (SUCCEEDED(item->GetValueAsInt32(AIMP_PLAYLISTITEM_PROPID_INDEX, &tempIndex)))
                {
                    songIndex = tempIndex;
                }

                json song = {
                    {"artist", artist},
                    {"duration", duration},
                    {"index", songIndex},
                    {"playlistId", playlistId},
                    {"title", title}
                };

                _results.push_back(song);
                _hasErrors = false;
                fileInfo->Release();
            }
            item->Release();
        }
    }
    queue->Release();
}