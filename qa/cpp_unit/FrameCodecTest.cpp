#include "FrameCodec.h"

#include <gtest/gtest.h>

TEST(FrameCodecTest, EncodesLengthBeforePayload)
{
    const char payload[] = "hello";
    std::vector<char> frame = FrameCodec::encode(payload, 5);
    ASSERT_EQ(frame.size(), 9u);

    int length = 0;
    ASSERT_TRUE(FrameCodec::decodeLength(frame.data(), 4, length));
    EXPECT_EQ(length, 5);
    EXPECT_EQ(std::string(frame.data() + 4, 5), "hello");
}

TEST(FrameCodecTest, RejectsInvalidLengths)
{
    int length = 0;
    int invalid = -1;
    EXPECT_FALSE(FrameCodec::decodeLength(reinterpret_cast<const char *>(&invalid), 4, length));
    EXPECT_TRUE(FrameCodec::encode(NULL, 5).empty());
    EXPECT_TRUE(FrameCodec::encode("x", FrameCodec::kDefaultMaxPayload + 1).empty());
}
