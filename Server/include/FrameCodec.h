#ifndef FRAME_CODEC_H
#define FRAME_CODEC_H

#include <cstddef>
#include <vector>

class FrameCodec
{
public:
    static const int kHeaderSize = 4;
    static const int kDefaultMaxPayload = 1024 * 1024;

    static bool decodeLength(const char *header, std::size_t headerSize,
                             int &payloadSize,
                             int maxPayload = kDefaultMaxPayload);
    static std::vector<char> encode(const char *payload, int payloadSize);
};

#endif
