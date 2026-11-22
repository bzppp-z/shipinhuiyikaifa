#include "FrameCodec.h"

#include <cstring>

bool FrameCodec::decodeLength(const char *header, std::size_t headerSize,
                              int &payloadSize, int maxPayload)
{
    payloadSize = 0;
    if (header == NULL || headerSize < sizeof(int) || maxPayload <= 0)
        return false;

    int decoded = 0;
    std::memcpy(&decoded, header, sizeof(decoded));
    if (decoded <= 0 || decoded > maxPayload)
        return false;

    payloadSize = decoded;
    return true;
}

std::vector<char> FrameCodec::encode(const char *payload, int payloadSize)
{
    if (payload == NULL || payloadSize <= 0 ||
            payloadSize > kDefaultMaxPayload)
        return std::vector<char>();

    std::vector<char> frame(sizeof(int) + payloadSize, 0);
    std::memcpy(frame.data(), &payloadSize, sizeof(payloadSize));
    std::memcpy(frame.data() + sizeof(payloadSize), payload, payloadSize);
    return frame;
}
