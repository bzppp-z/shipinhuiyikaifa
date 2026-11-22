#include "RoomManager.h"

#include <gtest/gtest.h>

TEST(RoomManagerTest, CreatesAndTracksMembers)
{
    RoomManager manager;
    ASSERT_TRUE(manager.create(100, 1));
    EXPECT_TRUE(manager.exists(100));
    EXPECT_TRUE(manager.addMember(100, 2));
    EXPECT_EQ(manager.members(100), (std::vector<int>{1, 2}));
}

TEST(RoomManagerTest, RejectsDuplicateAndUnknownMembers)
{
    RoomManager manager;
    ASSERT_TRUE(manager.create(100, 1));
    EXPECT_FALSE(manager.create(100, 2));
    EXPECT_FALSE(manager.addMember(999, 2));
    EXPECT_FALSE(manager.addMember(100, 1));
}

TEST(RoomManagerTest, RemovesEmptyRoom)
{
    RoomManager manager;
    ASSERT_TRUE(manager.create(100, 1));
    ASSERT_TRUE(manager.removeMember(100, 1));
    EXPECT_FALSE(manager.exists(100));
    EXPECT_TRUE(manager.empty());
}
