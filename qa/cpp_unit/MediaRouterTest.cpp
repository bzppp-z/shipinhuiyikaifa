#include "MediaRouter.h"

#include <gtest/gtest.h>

TEST(MediaRouterTest, ExcludesSender)
{
    const std::vector<int> members{1, 2, 3};
    EXPECT_EQ(MediaRouter::targets(members, 2), (std::vector<int>{1, 3}));
}

TEST(MediaRouterTest, KeepsEmptyResultForUnknownSender)
{
    const std::vector<int> members{1, 2};
    EXPECT_EQ(MediaRouter::targets(members, 9), members);
}
