#pragma once

#include "LocalFileDataProvider.h"

#include <string>
#include <memory>
#include <random>

// User id of items hosted locally in offline mode
#define OFFLINE_LOCAL_HOST_ID -1
// User id of items which were hosted by other users while online (valid in offline mode)
#define OFFLINE_REMOTE_HOST_ID -2

class PlaylistItem
{
public:
    int64_t GetItemId() const { return _itemId; }
    bool Initialized() { return !_dataProvider->Initializing(); } // TODO: Add const
    bool InitSuccess() { return !_dataProvider->InitFailed(); } // TODO: Add const

    std::unique_ptr<LocalFileDataProvider> CopyDataProvider() const { return std::make_unique<LocalFileDataProvider>(_dataProvider.get()); }
    LocalFileDataProvider* DataProvider() const { return _dataProvider.get(); }

    std::wstring GetCustomStatus() const { return _statusStr; }
    void SetCustomStatus(std::wstring statusString) { _statusStr = statusString; }
    std::wstring GetFilename() const { return _filename; }
    void SetFilename(std::wstring filename) { _filename = filename; }
    Duration GetDuration() // TODO: Add const
    {
        if (!_dataProvider)
            return _duration;

        if (_duration == -1)
        {
            if (Initialized())
            {
                _duration = _dataProvider->MediaDuration();
            }
        }
        if (_duration != -1)
            return _duration;
        else
            return 0;
    }
    void SetDuration(Duration duration) { _duration = duration; }
    int64_t GetMediaId() const { return _mediaId; }
    void SetMediaId(int64_t id) { _mediaId = id; }
    int64_t GetUserId() const { return _userId; }
    void SetUserId(int64_t id) { _userId = id; }

    PlaylistItem();
    PlaylistItem(std::wstring path);
    ~PlaylistItem();
    PlaylistItem(PlaylistItem&&) = delete;
    PlaylistItem& operator=(PlaylistItem&&) = delete;
    PlaylistItem(const PlaylistItem&) = delete;
    PlaylistItem& operator=(const PlaylistItem&) = delete;

private:
    bool _initDone = false;
    bool _initSuccess = false;
    std::unique_ptr<LocalFileDataProvider> _dataProvider = nullptr;
    std::wstring _filename = L"";
    Duration _duration = -1;

    std::wstring _statusStr = L"";

    int64_t _mediaId = -1;
    int64_t _userId = -1;

    // Item id generation

    static std::mutex _ENGINE_MUTEX;
    static bool _ENGINE_INIT;
    static std::mt19937_64 _ENGINE;
    static std::uniform_int_distribution<uint64_t> _DISTRIBUTION;
    static int64_t _GenerateItemId();
    int64_t _itemId;
};