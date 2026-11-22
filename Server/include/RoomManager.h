#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <map>
#include <mutex>
#include <vector>

class RoomManager
{
public:
    bool create(int roomId, int ownerId);
    bool exists(int roomId) const;
    bool addMember(int roomId, int userId);
    bool removeMember(int roomId, int userId);
    std::vector<int> members(int roomId) const;
    bool empty() const;

private:
    mutable std::mutex m_mutex;
    std::map<int, std::vector<int> > m_rooms;
};

#endif
