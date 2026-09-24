#include "pch.h"
#include "../tasks/CMoveSongQueueTask.h"

CMoveSongQueueTask::CMoveSongQueueTask(MyPlugin* plugin, const int& songIndex, const int& targetIndex)
    : _plugin(plugin), _songIndex(songIndex), _targetIndex(targetIndex) {
}

HRESULT WINAPI CMoveSongQueueTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CMoveSongQueueTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CMoveSongQueueTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CMoveSongQueueTask::Execute(IAIMPTaskOwner* Owner)
{
    IAIMPPlaylistQueue* queue = nullptr;
    if (FAILED(_plugin->GetCore()->QueryInterface(IID_IAIMPPlaylistQueue, (void**)&queue))) return;

    queue->Move2(_songIndex, _targetIndex);
    _hasErrors = false;
    queue->Release();
}