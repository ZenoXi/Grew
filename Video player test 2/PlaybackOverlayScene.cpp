#include "App.h" // App.h must be included first
#include "PlaybackOverlayScene.h"

#include "NetworkInterfaceNew.h"
#include "MediaReceiverDataProvider.h"
#include "MediaHostDataProvider.h"

PlaybackOverlayScene::PlaybackOverlayScene()
{
    _playbackScene = (PlaybackScene*)App::Instance()->FindScene(PlaybackScene::StaticName());
}

void PlaybackOverlayScene::_Init(const SceneOptionsBase* options)
{
    PlaybackOverlaySceneOptions opt;
    if (options)
    {
        opt = *reinterpret_cast<const PlaybackOverlaySceneOptions*>(options);
    }
    
    _playbackQueuePanel = std::make_unique<zcom::Panel>();
    _playbackQueuePanel->SetParentHeightPercent(1.0f);
    _playbackQueuePanel->SetBaseSize(600, -110);
    _playbackQueuePanel->SetOffsetPixels(40, 40);
    _playbackQueuePanel->SetBorderVisibility(true);
    _playbackQueuePanel->SetBorderColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
    _playbackQueuePanel->SetBackgroundColor(D2D1::ColorF(0.1f, 0.1f, 0.1f));

    //_mediaQueueItem = new zcom::MediaQueueItem(L"E:\\aots4e5.mkv");
    //_mediaQueueItem->SetBaseSize(200, 35);
    //_mediaQueueItem->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f));

    _addFileButton = std::make_unique<zcom::Button>(L"Add file");
    _addFileButton->SetBaseSize(100, 25);
    _addFileButton->SetOffsetPixels(40, -40);
    _addFileButton->SetVerticalAlignment(zcom::Alignment::END);
    _addFileButton->SetBorderVisibility(true);
    _addFileButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _addFileButton->SetOnActivated([&]()
    {
        _addingFile = true;
        _fileDialog = new AsyncFileDialog();
        _fileDialog->Open();
    });

    _closeOverlayButton = std::make_unique<zcom::Button>(L"Close");
    _closeOverlayButton->SetBaseSize(100, 25);
    _closeOverlayButton->SetOffsetPixels(-40, 40);
    _closeOverlayButton->SetHorizontalAlignment(zcom::Alignment::END);
    _closeOverlayButton->SetBorderVisibility(true);
    _closeOverlayButton->SetActivation(zcom::ButtonActivation::RELEASE);
    _closeOverlayButton->SetOnActivated([&]()
    {
        App::Instance()->MoveSceneToBack(this->GetName());
    });

    _networkStatusLabel = std::make_unique<zcom::Label>(L"");
    _networkStatusLabel->SetBaseSize(300, 25);
    _networkStatusLabel->SetOffsetPixels(-40, 80);
    _networkStatusLabel->SetHorizontalAlignment(zcom::Alignment::END);
    _networkStatusLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _networkStatusLabel->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));

    _connectedUserLabel = std::make_unique<zcom::Label>(L"");
    _connectedUserLabel->SetBaseSize(300, 25);
    _connectedUserLabel->SetOffsetPixels(-40, 120);
    _connectedUserLabel->SetHorizontalAlignment(zcom::Alignment::END);
    _connectedUserLabel->SetVerticalTextAlignment(zcom::Alignment::CENTER);
    _connectedUserLabel->SetBackgroundColor(D2D1::ColorF(0.25f, 0.25f, 0.25f));

    _canvas->AddComponent(_playbackQueuePanel.get());
    _canvas->AddComponent(_addFileButton.get());
    _canvas->AddComponent(_closeOverlayButton.get());
    _canvas->AddComponent(_networkStatusLabel.get());
    _canvas->AddComponent(_connectedUserLabel.get());
    _canvas->SetBackgroundColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.75f));

    // Set up packet receivers
    _SetUpPacketReceivers(znet::NetworkInterface::Instance()->Mode());
}

void PlaybackOverlayScene::_Uninit()
{

}

void PlaybackOverlayScene::_Focus()
{

}

void PlaybackOverlayScene::_Unfocus()
{

}

void PlaybackOverlayScene::_Update()
{
    _playbackQueuePanel->Update();

    znet::NetworkMode netMode = znet::NetworkInterface::Instance()->Mode();
    if (netMode != _networkMode)
    {
        _networkMode = netMode;
        _SetUpPacketReceivers(netMode);
    }

    // File dialog
    _CheckFileDialogCompletion();

    // Manage queue items
    _CheckForItemAddRequest();
    _CheckForItemAdd();
    _CheckForItemRemoveRequest();
    _CheckForItemRemove();
    _CheckForStartRequest();
    _CheckForStartOrder();
    _CheckForStartResponse();
    _CheckForStart();
    _CheckForStopRequest();
    _CheckForStop();
    bool changed = false;
    changed |= _ManageLoadingItems();
    changed |= _ManageReadyItems();
    if (changed) _RearrangeQueuePanel();

    // Autoplay
    _PlayNextItem();

    // Update network and user labels
    std::string statusString = znet::NetworkInterface::Instance()->StatusString();
    _networkStatusLabel->SetText(string_to_wstring(statusString));

    auto users = znet::NetworkInterface::Instance()->Users();
    std::ostringstream ss("");
    for (int i = 0; i < users.size(); i++)
    {
        ss << users[i].id << " ";
    }
    _connectedUserLabel->SetText(string_to_wstring(ss.str()));
}

void PlaybackOverlayScene::_SetUpPacketReceivers(znet::NetworkMode netMode)
{
    _queueAddRequestReceiver.reset(nullptr);
    _queueAddReceiver.reset(nullptr);
    _queueRemoveRequestReceiver.reset(nullptr);
    _queueRemoveReceiver.reset(nullptr);
    _playbackStartRequestReceiver.reset(nullptr);
    _playbackStartOrderReceiver.reset(nullptr);
    _playbackStartResponseReceiver.reset(nullptr);
    _playbackStartReceiver.reset(nullptr);
    _playbackStopRequestReceiver.reset(nullptr);
    _playbackStopReceiver.reset(nullptr);
    if (netMode == znet::NetworkMode::CLIENT)
    {
        _queueAddReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::QUEUE_ITEM_ADD);
        _queueRemoveReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::QUEUE_ITEM_REMOVE);
        _playbackStopReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP);
    }
    else if (netMode == znet::NetworkMode::SERVER)
    {
        _queueAddRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::QUEUE_ITEM_ADD_REQUEST);
        _queueRemoveRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::QUEUE_ITEM_REMOVE_REQUEST);
        _playbackStartRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_REQUEST);
        _playbackStartResponseReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_RESPONSE);
        _playbackStopRequestReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_STOP_REQUEST);
    }

    if (netMode != znet::NetworkMode::OFFLINE)
    {
        _playbackStartOrderReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START_ORDER);
        _playbackStartReceiver = std::make_unique<znet::PacketReceiver>(znet::PacketType::PLAYBACK_START);
    }
}

void PlaybackOverlayScene::_CheckFileDialogCompletion()
{
    if (_addingFile)
    {
        if (_fileDialog->Done())
        {
            if (_fileDialog->Result() != L"")
            {
                auto newItem = std::make_unique<zcom::MediaQueueItem>(_fileDialog->Result());
                newItem->SetParentWidthPercent(1.0f);
                newItem->SetBaseHeight(25);
                _loadingItems.push_back(std::move(newItem));
                _RearrangeQueuePanel();
            }
            _addingFile = false;
            delete _fileDialog;
        }
    }
}

void PlaybackOverlayScene::_CheckForItemAddRequest()
{
    if (!_queueAddRequestReceiver)
        return;

    while (_queueAddRequestReceiver->PacketCount() > 0)
    {
        // Process packet
        struct RequestPacket
        {
            int32_t callbackId;
            int64_t mediaDuration;
        };
        auto packetPair = _queueAddRequestReceiver->GetPacket();
        int32_t callbackId = *(int32_t*)(packetPair.first.Bytes());
        int64_t mediaDuration = *(int64_t*)(packetPair.first.Bytes() + 4);
        size_t bytesWithoutFilename = sizeof(int32_t) + sizeof(int64_t);
        size_t filenameLength = (packetPair.first.size - bytesWithoutFilename) / 2;
        std::wstring filename;
        filename.resize(filenameLength);
        for (int i = 0; i < filenameLength; i++)
            filename[i] = ((wchar_t*)(packetPair.first.Bytes() + bytesWithoutFilename))[i];

        // Create item
        int32_t mediaId = _GenerateMediaId();
        int64_t userId = packetPair.second;
        auto newItem = std::make_unique<zcom::MediaQueueItem>();
        newItem->SetParentWidthPercent(1.0f);
        newItem->SetBaseHeight(25);
        newItem->SetMediaId(mediaId);
        newItem->SetUserId(userId);
        newItem->SetDuration(Duration(mediaDuration));
        newItem->SetFilename(filename);

        // Add item to queue
        _readyItems.push_back(std::move(newItem));
        _RearrangeQueuePanel();

        // Send item to all clients
        size_t packetSize = sizeof(int32_t) + sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t) + sizeof(wchar_t) * filename.length();
        auto bytes = std::make_unique<int8_t[]>(packetSize);
        *((int32_t*)bytes.get()) = mediaId;
        *((int32_t*)(bytes.get() + 4)) = callbackId;
        *((int64_t*)(bytes.get() + 8)) = userId;
        *((int64_t*)(bytes.get() + 16)) = mediaDuration;
        std::copy_n(filename.begin(), filename.length(), (wchar_t*)(bytes.get() + 24));

        std::vector<int64_t> destinationUsers;
        auto users = znet::NetworkInterface::Instance()->Users();
        for (auto& user : users)
            destinationUsers.push_back(user.id);

        znet::NetworkInterface::Instance()->Send(znet::Packet(std::move(bytes), packetSize, (int)znet::PacketType::QUEUE_ITEM_ADD), destinationUsers);
    }
}

void PlaybackOverlayScene::_CheckForItemAdd()
{
    if (!_queueAddReceiver)
        return;

    while (_queueAddReceiver->PacketCount() > 0)
    {
        struct ItemPacket
        {
            int32_t mediaId;
            int32_t callbackId;
            int64_t mediaHostId;
            int64_t mediaDuration;
        };
        auto packetPair = _queueAddReceiver->GetPacket();
        ItemPacket itemPacket = packetPair.first.Cast<ItemPacket>();

        if (itemPacket.mediaHostId == znet::NetworkInterface::Instance()->ThisUser().id)
        {
            for (int i = 0; i < _loadingItems.size(); i++)
            {
                if (_loadingItems[i]->GetCallbackId() == itemPacket.callbackId)
                {
                    if (itemPacket.mediaId != -1)
                    {
                        // Move confirmed item
                        _loadingItems[i]->SetMediaId(itemPacket.mediaId);
                        _loadingItems[i]->SetUserId(itemPacket.mediaHostId);
                        _loadingItems[i]->SetCustomStatus(L"");
                        _readyItems.push_back(std::move(_loadingItems[i]));
                        _readyItems.back()->SetButtonVisibility(zcom::MediaQueueItem::BTN_PLAY | zcom::MediaQueueItem::BTN_DELETE);
                        _loadingItems.erase(_loadingItems.begin() + i);
                        _RearrangeQueuePanel();
                    }
                    else
                    {
                        _loadingItems[i]->SetMediaId(-2);
                    }
                    break;
                }
            }
        }
        else
        {
            // Extract filename
            size_t filenameLength = (packetPair.first.size - sizeof(ItemPacket)) / 2;
            std::wstring filename;
            filename.resize(filenameLength);
            for (int i = 0; i < filenameLength; i++)
                filename[i] = ((wchar_t*)(packetPair.first.Bytes() + sizeof(ItemPacket)))[i];

            // Create item
            auto newItem = std::make_unique<zcom::MediaQueueItem>();
            newItem->SetParentWidthPercent(1.0f);
            newItem->SetBaseHeight(25);
            newItem->SetMediaId(itemPacket.mediaId);
            newItem->SetUserId(itemPacket.mediaHostId);
            newItem->SetDuration(Duration(itemPacket.mediaDuration));
            newItem->SetFilename(filename);
            newItem->SetButtonVisibility(zcom::MediaQueueItem::BTN_DELETE | zcom::MediaQueueItem::BTN_PLAY);

            // Add item to queue
            _readyItems.push_back(std::move(newItem));
            _RearrangeQueuePanel();
        }
    }
}

void PlaybackOverlayScene::_CheckForItemRemoveRequest()
{
    if (!_queueRemoveRequestReceiver)
        return;

    while (_queueRemoveRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _queueRemoveRequestReceiver->GetPacket();
        int32_t mediaId = packetPair.first.Cast<int32_t>();

        // Remove item
        bool itemRemoved = false;
        for (int i = 0; i < _readyItems.size(); i++)
        {
            if (_readyItems[i]->GetMediaId() == mediaId
             && _readyItems[i]->GetMediaId() != _currentlyPlaying)
            {
                _readyItems.erase(_readyItems.begin() + i);
                itemRemoved = true;
            }
        }

        if (itemRemoved)
        {
            // Send remove order to all clients
            std::vector<int64_t> destinationUsers;
            auto users = znet::NetworkInterface::Instance()->Users();
            for (auto& user : users)
                destinationUsers.push_back(user.id);
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::QUEUE_ITEM_REMOVE).From(mediaId), destinationUsers);

            _RearrangeQueuePanel();
        }
    }
}

void PlaybackOverlayScene::_CheckForItemRemove()
{
    if (!_queueRemoveReceiver)
        return;

    while (_queueRemoveReceiver->PacketCount() > 0)
    {
        auto packetPair = _queueRemoveReceiver->GetPacket();
        int32_t mediaId = packetPair.first.Cast<int32_t>();

        for (int i = 0; i < _readyItems.size(); i++)
        {
            if (_readyItems[i]->GetMediaId() == mediaId)
            {
                _readyItems.erase(_readyItems.begin() + i);
                _RearrangeQueuePanel();
            }
        }
    }
}

void PlaybackOverlayScene::_CheckForStartRequest()
{
    if (!_playbackStartRequestReceiver)
        return;

    while (_playbackStartRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStartRequestReceiver->GetPacket();
        int32_t mediaId = packetPair.first.Cast<int32_t>();

        if (_currentlyPlaying != -1 || _loadingPlayback != -1)
            continue;

        // Find if item exists
        int64_t userId = -1;
        int itemIndex = -1;
        for (int i = 0; i < _readyItems.size(); i++)
        {
            if (_readyItems[i]->GetMediaId() == mediaId)
            {
                userId = _readyItems[i]->GetUserId();
                itemIndex = i;
                break;
            }
        }
        if (itemIndex != -1)
        {
            _loadingPlayback = mediaId;

            // Send playback order to media host
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_ORDER).From(mediaId), { userId });
        }
    }
}

void PlaybackOverlayScene::_CheckForStartOrder()
{
    if (!_playbackStartOrderReceiver)
        return;

    while (_playbackStartOrderReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStartOrderReceiver->GetPacket();
        int32_t mediaId = packetPair.first.Cast<int32_t>();

        // Find if item exists
        int itemIndex = -1;
        for (int i = 0; i < _readyItems.size(); i++)
        {
            if (_readyItems[i]->GetMediaId() == mediaId)
            {
                itemIndex = i;
                break;
            }
        }
        if (itemIndex != -1)
        {
            // Start media playback
            _currentlyPlaying = mediaId;

            PlaybackSceneOptions options;
            options.dataProvider = new MediaHostDataProvider(_readyItems[itemIndex]->CopyDataProvider());
            options.playbackMode = PlaybackMode::SERVER;
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
            _RearrangeQueuePanel();

            // Notify server that playback has started
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_RESPONSE).From(int8_t(1)), { 0 });
        }
        else
        {
            // Notify server that playback was declined
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_RESPONSE).From(int8_t(0)), { 0 });
        }
    }
}

void PlaybackOverlayScene::_CheckForStartResponse()
{
    if (!_playbackStartResponseReceiver)
        return;

    while (_playbackStartResponseReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStartResponseReceiver->GetPacket();
        int8_t response = packetPair.first.Cast<int8_t>();

        if (_loadingPlayback < 0)
            continue;

        int itemIndex = -1;
        for (int i = 0; i < _readyItems.size(); i++)
        {
            if (_readyItems[i]->GetMediaId() == _loadingPlayback)
            {
                itemIndex = i;
                break;
            }
        }
        if (itemIndex == -1)
            continue;

        // Notify everyone of playback start (including the server itself, except if it is the host)
        std::vector<int64_t> destinationUsers;
        auto users = znet::NetworkInterface::Instance()->Users();
        for (auto& user : users)
        {
            if (_readyItems[itemIndex]->GetUserId() != user.id)
            {
                destinationUsers.push_back(user.id);
            }
        }
        if (_readyItems[itemIndex]->GetUserId() != 0)
            destinationUsers.push_back(0);

        struct PlaybackStartDesc
        {
            int32_t mediaId;
            int64_t hostId;
        };
        PlaybackStartDesc startDesc = { _loadingPlayback, _readyItems[itemIndex]->GetUserId() };
        znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START).From(startDesc), destinationUsers);

        if (_readyItems[itemIndex]->GetUserId() == 0)
            _loadingPlayback = -1;
    }
}

void PlaybackOverlayScene::_CheckForStart()
{
    if (!_playbackStartReceiver)
        return;

    while (_playbackStartReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStartReceiver->GetPacket();
        struct PlaybackStartDesc
        {
            int32_t mediaId;
            int64_t hostId;
        };
        auto startDesc = packetPair.first.Cast<PlaybackStartDesc>();

        if (_loadingPlayback != -1)
            _loadingPlayback = -1;
        _currentlyPlaying = startDesc.mediaId;

        PlaybackSceneOptions options;
        options.dataProvider = new MediaReceiverDataProvider(startDesc.hostId);
        options.playbackMode = PlaybackMode::CLIENT;
        App::Instance()->UninitScene(PlaybackScene::StaticName());
        App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
        App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
        _RearrangeQueuePanel();
    }
}

void PlaybackOverlayScene::_CheckForStopRequest()
{
    if (!_playbackStopRequestReceiver)
        return;

    while (_playbackStopRequestReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStopRequestReceiver->GetPacket();

        if (_currentlyPlaying != -1)
        {
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            _currentlyPlaying = -1;

            // Sent playback stop order to all clients
            std::vector<int64_t> destinationUsers;
            auto users = znet::NetworkInterface::Instance()->Users();
            for (auto& user : users)
                destinationUsers.push_back(user.id);
            znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP), destinationUsers);
            _RearrangeQueuePanel();
        }
    }
}

void PlaybackOverlayScene::_CheckForStop()
{
    if (!_playbackStopReceiver)
        return;

    while (_playbackStopReceiver->PacketCount() > 0)
    {
        auto packetPair = _playbackStopReceiver->GetPacket();

        if (_currentlyPlaying != -1)
        {
            App::Instance()->UninitScene(PlaybackScene::StaticName());
            _currentlyPlaying = -1;
            _RearrangeQueuePanel();
        }
    }
}

bool PlaybackOverlayScene::_ManageLoadingItems()
{
    static const wchar_t* customStatusMsgWaiting = L"Waiting for server..";
    static const wchar_t* customStatusMsgDenied = L"Playback denied.";

    znet::NetworkMode netMode = znet::NetworkInterface::Instance()->Mode();

    bool changed = false;
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        // Delete
        if (_loadingItems[i]->Delete())
        {
            _loadingItems.erase(_loadingItems.begin() + i);
            i--;
            changed = true;
            continue;
        }

        // Move from loading to ready
        if (_loadingItems[i]->Initialized() && _loadingItems[i]->InitSuccess())
        {
            // Request media id (client mode)
            if (netMode == znet::NetworkMode::CLIENT)
            {
                if (_loadingItems[i]->GetCustomStatus() != customStatusMsgWaiting
                 && _loadingItems[i]->GetCustomStatus() != customStatusMsgDenied)
                {
                    _loadingItems[i]->SetCustomStatus(customStatusMsgWaiting);

                    // Send queue add request
                    std::wstring filename = _loadingItems[i]->GetFilename();
                    size_t packetSize = sizeof(int32_t) + sizeof(int64_t) + sizeof(wchar_t) * filename.length();
                    auto bytes = std::make_unique<int8_t[]>(packetSize);
                    *((int32_t*)bytes.get()) = _loadingItems[i]->GetCallbackId();
                    *((int64_t*)(bytes.get() + 4)) = _loadingItems[i]->DataProvider()->MediaDuration().GetTicks();
                    std::copy_n(filename.begin(), filename.length(), (wchar_t*)(bytes.get() + 12));
                    
                    znet::NetworkInterface::Instance()->Send(znet::Packet(std::move(bytes), packetSize, (int)znet::PacketType::QUEUE_ITEM_ADD_REQUEST), { 0 });
                }
                // Set item status as denied
                else if (_loadingItems[i]->GetMediaId() == -2)
                {
                    if (_loadingItems[i]->GetCustomStatus() != customStatusMsgDenied)
                    {
                        _loadingItems[i]->SetCustomStatus(customStatusMsgDenied);
                    }
                }
            }
            else
            {
                int32_t mediaId = _GenerateMediaId();
                _loadingItems[i]->SetMediaId(mediaId);

                if (netMode == znet::NetworkMode::SERVER)
                {
                    _loadingItems[i]->SetUserId(0);

                    // Send item to all clients
                    std::wstring filename = _loadingItems[i]->GetFilename();
                    size_t packetSize = sizeof(int32_t) + sizeof(int32_t) + sizeof(int64_t) + sizeof(int64_t) + sizeof(wchar_t) * filename.length();
                    auto bytes = std::make_unique<int8_t[]>(packetSize);
                    *((int32_t*)bytes.get()) = mediaId;
                    *((int32_t*)(bytes.get() + 4)) = 0;
                    *((int64_t*)(bytes.get() + 8)) = 0;
                    *((int64_t*)(bytes.get() + 16)) = _loadingItems[i]->DataProvider()->MediaDuration().GetTicks();
                    std::copy_n(filename.begin(), filename.length(), (wchar_t*)(bytes.get() + 24));

                    std::vector<int64_t> destinationUsers;
                    auto users = znet::NetworkInterface::Instance()->Users();
                    for (auto& user : users)
                        destinationUsers.push_back(user.id);

                    znet::NetworkInterface::Instance()->Send(znet::Packet(std::move(bytes), packetSize, (int)znet::PacketType::QUEUE_ITEM_ADD), destinationUsers);
                }

                _readyItems.push_back(std::move(_loadingItems[i]));
                _readyItems.back()->SetButtonVisibility(zcom::MediaQueueItem::BTN_PLAY | zcom::MediaQueueItem::BTN_DELETE);
                _loadingItems.erase(_loadingItems.begin() + i);
                i--;
                changed = true;
                continue;
            }
        }
    }
    return changed;
}

bool PlaybackOverlayScene::_ManageReadyItems()
{
    znet::NetworkMode netMode = znet::NetworkInterface::Instance()->Mode();

    bool changed = false;
    for (int i = 0; i < _readyItems.size(); i++)
    {
        // Play
        if (_readyItems[i]->Play())
        {
            if (netMode != znet::NetworkMode::OFFLINE)
            {
                int32_t mediaId = _readyItems[i]->GetMediaId();
                // Send play request to server
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_START_REQUEST).From(mediaId), { 0 });
            }
            else
            {
                _currentlyPlaying = _readyItems[i]->GetMediaId();
                PlaybackSceneOptions options;
                options.dataProvider = _readyItems[i]->CopyDataProvider();
                options.playbackMode = PlaybackMode::OFFLINE;
                App::Instance()->UninitScene(PlaybackScene::StaticName());
                App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
                App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
                changed = true;
            }
        }
        // Stop
        else if (_readyItems[i]->Stop())
        {
            if (netMode == znet::NetworkMode::CLIENT)
            {
                // Send stop request to server
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP_REQUEST), { 0 });
            }
            else
            {

                if (netMode == znet::NetworkMode::SERVER)
                {
                    // Send stop order to all clients
                    std::vector<int64_t> destinationUsers;
                    auto users = znet::NetworkInterface::Instance()->Users();
                    for (auto& user : users)
                        destinationUsers.push_back(user.id);
                    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::PLAYBACK_STOP), destinationUsers);
                }

                // Stop playback
                _currentlyPlaying = -1;
                App::Instance()->UninitScene(PlaybackScene::StaticName());
                changed = true;
            }
        }
        // Delete
        else if (_readyItems[i]->Delete())
        {
            if (netMode == znet::NetworkMode::CLIENT)
            {
                int32_t mediaId = _readyItems[i]->GetMediaId();
                // Send delete request to server
                znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::QUEUE_ITEM_REMOVE_REQUEST).From(mediaId), { 0 });
            }
            else
            {
                if (netMode == znet::NetworkMode::SERVER)
                {
                    int32_t mediaId = _readyItems[i]->GetMediaId();

                    // Send remove order to all clients
                    std::vector<int64_t> destinationUsers;
                    auto users = znet::NetworkInterface::Instance()->Users();
                    for (auto& user : users)
                        destinationUsers.push_back(user.id);
                    znet::NetworkInterface::Instance()->Send(znet::Packet((int)znet::PacketType::QUEUE_ITEM_REMOVE).From(mediaId), destinationUsers);
                }

                _readyItems.erase(_readyItems.begin() + i);
                i--;
                changed = true;
                continue;
            }
        }
    }
    return changed;
}

void PlaybackOverlayScene::_PlayNextItem()
{
    znet::NetworkMode netMode = znet::NetworkInterface::Instance()->Mode();

    if (((_autoplay && _playbackScene->Finished()) || _waiting)
        && netMode == znet::NetworkMode::OFFLINE)
    {
        bool uninitScene = true;
        _currentlyPlaying++;
        if (_currentlyPlaying == _readyItems.size())
        {
            _currentlyPlaying = -1;
            if (_waiting)
                uninitScene = false;
        }

        Scene* scene = nullptr;
        bool focused = false;
        if (uninitScene)
        {
            scene = App::Instance()->FindActiveScene(PlaybackScene::StaticName());
            if (scene)
            {
                focused = scene->Focused();
                App::Instance()->UninitScene(PlaybackScene::StaticName());
            }
        }

        if (_currentlyPlaying != -1)
        {
            _waiting = false;
            PlaybackSceneOptions options;
            options.dataProvider = _readyItems[_currentlyPlaying]->CopyDataProvider();
            options.playbackMode = PlaybackMode::OFFLINE;
            App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
            if (focused)
                App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
            else
                App::Instance()->MoveSceneBehind(PlaybackScene::StaticName(), StaticName());
        }

        if (uninitScene)
            _RearrangeQueuePanel();
    }
}

ID2D1Bitmap1* PlaybackOverlayScene::_Draw(Graphics g)
{
    // Draw UI
    ID2D1Bitmap* bitmap = _canvas->Draw(g);
    g.target->DrawBitmap(bitmap);

    return nullptr;
}

void PlaybackOverlayScene::_Resize(int width, int height)
{

}

void PlaybackOverlayScene::AddItem(std::wstring filepath)
{
    auto newItem = std::make_unique<zcom::MediaQueueItem>(filepath);
    newItem->SetParentWidthPercent(1.0f);
    newItem->SetBaseHeight(25);
    _loadingItems.push_back(std::move(newItem));
    _RearrangeQueuePanel();

    //_readyItems.push_back(std::move(newItem));
    //_currentlyPlaying = _readyItems.size() - 1;

    //PlaybackSceneOptions options;
    //options.fileName = wstring_to_string(filepath);
    //options.mode = PlaybackMode::OFFLINE;
    //App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
    //App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
}

void PlaybackOverlayScene::WaitForLoad(bool focus)
{
    if (_readyItems.empty())
    {
        _waiting = true;
        _currentlyPlaying = -1;

        PlaybackSceneOptions options;
        options.placeholder = true;
        App::Instance()->InitScene(PlaybackScene::StaticName(), &options);
        if (focus)
            App::Instance()->MoveSceneToFront(PlaybackScene::StaticName());
    }
}

void PlaybackOverlayScene::SetAutoplay(bool autoplay)
{
    _autoplay = autoplay;
}

bool PlaybackOverlayScene::GetAutoplay() const
{
    return _autoplay;
}

void PlaybackOverlayScene::_RearrangeQueuePanel()
{
    _playbackQueuePanel->ClearItems();
    for (int i = 0; i < _readyItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_readyItems[i].get());
        _readyItems[i]->SetVerticalOffsetPixels(25 * i);
        _readyItems[i]->SetBackgroundColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * (i % 2)));
        if (_readyItems[i]->GetMediaId() == _currentlyPlaying)
        {
            _readyItems[i]->SetBackgroundColor(D2D1::ColorF(D2D1::ColorF::Orange, 0.2f));
            _readyItems[i]->SetButtonVisibility(zcom::MediaQueueItem::BTN_STOP);
            //_readyItems[i]->SetNowPlaying(true);
        }
        else
        {
            _readyItems[i]->SetButtonVisibility(zcom::MediaQueueItem::BTN_PLAY | zcom::MediaQueueItem::BTN_DELETE);
            //_readyItems[i]->SetNowPlaying(false);
        }

    }
    for (int i = 0; i < _loadingItems.size(); i++)
    {
        _playbackQueuePanel->AddItem(_loadingItems[i].get());
        _loadingItems[i]->SetVerticalOffsetPixels(25 * (i + _readyItems.size()));
    }
    _playbackQueuePanel->Resize();
}