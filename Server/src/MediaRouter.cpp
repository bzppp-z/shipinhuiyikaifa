#include "MediaRouter.h"

std::vector<int> MediaRouter::targets(const std::vector<int> &members, int senderId)
{
    std::vector<int> result;
    for (std::vector<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it != senderId)
            result.push_back(*it);
    }
    return result;
}
