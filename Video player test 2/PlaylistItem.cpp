#include "PlaylistItem.h"

#include "Functions.h"

std::mutex PlaylistItem::_ENGINE_MUTEX;
bool PlaylistItem::_ENGINE_INIT = false;
std::mt19937_64 PlaylistItem::_ENGINE;
std::uniform_int_distribution<uint64_t> PlaylistItem::_DISTRIBUTION(0, std::numeric_limits<int64_t>::max());

int64_t PlaylistItem::_GenerateItemId()
{
    std::lock_guard<std::mutex> lock(_ENGINE_MUTEX);
    if (!_ENGINE_INIT)
    {
        std::random_device rnd;
        _ENGINE.seed(rnd());
        _ENGINE_INIT = true;
    }
    return _DISTRIBUTION(_ENGINE);
}

PlaylistItem::PlaylistItem()
{
    _itemId = _GenerateItemId();
}

PlaylistItem::PlaylistItem(std::wstring path) : PlaylistItem()
{
    // Extract only the filename
    _filename = L"";
    for (int i = path.length() - 1; i >= 0; i--)
    {
        if (path[i] == '\\' || path[i] == '/')
        {
            break;
        }
        _filename += path[i];
    }
    std::reverse(_filename.begin(), _filename.end());

    // Start processing the file
    _dataProvider = std::make_unique<LocalFileDataProvider>(wstring_to_string(path));
}

PlaylistItem::~PlaylistItem()
{

}