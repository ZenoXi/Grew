#pragma once

#include "ISerializable.h"
#include "MediaStream.h"

#include <vector>

class MediaMetadata : public ISerializable
{
public:
    std::vector<MediaStream> streams;

    SerializedData Serialize() const
    {
        std::vector<SerializedData> serializedStreams;
        serializedStreams.reserve(streams.size());

        size_t totalSize = 0;
        for (int i = 0; i < streams.size(); i++)
        {
            serializedStreams.push_back(streams[i].Serialize());
            totalSize += serializedStreams.back().Size();
        }

        // Combine all streams
        auto bytes = std::make_unique<uchar[]>(totalSize);
        size_t memPos = 0;
        for (int i = 0; i < serializedStreams.size(); i++)
        {
            std::copy_n(serializedStreams[i].Bytes(), serializedStreams[i].Size(), bytes.get() + memPos);
            memPos += serializedStreams[i].Size();
        }

        return { totalSize, bytes };
    }

    size_t Deserialize(SerializedData data)
    {
        streams.clear();

        size_t totalUsedBytes = 0;
        while (totalUsedBytes < data.Size())
        {
            MediaStream stream;
            size_t usedBytes = stream.Deserialize(data.Copy(totalUsedBytes));
            if (usedBytes == 0)
            {
                break;
            }
            totalUsedBytes += usedBytes;
            streams.push_back(std::move(stream));
        }

        return totalUsedBytes;
    }
};