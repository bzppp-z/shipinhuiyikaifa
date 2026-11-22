#ifndef MEDIA_ROUTER_H
#define MEDIA_ROUTER_H

#include <vector>

class MediaRouter
{
public:
    static std::vector<int> targets(const std::vector<int> &members, int senderId);
};

#endif
