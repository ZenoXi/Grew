#pragma once

#include "ISerializable.h"
#include "GameTime.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <string>
#include <stdexcept>

class MediaChapter : public ISerializable
{
public:
    int id;
    TimePoint start;
    TimePoint end;
    std::string title;

    MediaChapter() {}
    MediaChapter(int id, TimePoint start, TimePoint end, std::string title)
        : id(id), start(start), end(end), title(title) {}

    SerializedData Serialize() const
    {
        uint8_t stringLength;
        if (title.length() > 255)
            stringLength = 255;
        else
            stringLength = title.length();

        // Chapter class size: 4(int) + 8(int64_t) * 2 + 1(uint8_t) + stringLength (max: 255)
        size_t totalSize = 21 + (size_t)stringLength;
        auto data = std::make_unique<uchar[]>(totalSize);

        *(int*)(data.get()) = id;
        *(int64_t*)(data.get() + 4) = start.GetTicks();
        *(int64_t*)(data.get() + 12) = end.GetTicks();
        *(uint8_t*)(data.get() + 20) = stringLength;
        std::copy_n(title.data(), title.length(), data.get() + 21);

        return { totalSize, std::move(data) };
    }

    size_t Deserialize(SerializedData data)
    {
        if (data.Size() < 21)
            return 0;

        id = *(int*)(data.Bytes() + 0);
        start = *(int64_t*)(data.Bytes() + 4);
        end = *(int64_t*)(data.Bytes() + 12);
        uint8_t length = *(uint8_t*)(data.Bytes() + 20);
        if (length != 0 && data.Size() >= 21 + (size_t)length)
        {
            title.resize(length);
            std::copy_n(data.Bytes() + 21, length, (uchar*)title.data());
            return 21 + (size_t)length;
        }
        return 21;
    }
};