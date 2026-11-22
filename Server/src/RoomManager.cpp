#include "RoomManager.h"

#include <algorithm>

bool RoomManager::create(int roomId, int ownerId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (roomId <= 0 || ownerId <= 0 || m_rooms.count(roomId) != 0)
        return false;
    m_rooms[roomId].push_back(ownerId);
    return true;
}

bool RoomManager::exists(int roomId) const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_rooms.count(roomId) != 0;
}

bool RoomManager::addMember(int roomId, int userId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::map<int, std::vector<int> >::iterator room = m_rooms.find(roomId);
    if (room == m_rooms.end() || userId <= 0)
        return false;
    if (std::find(room->second.begin(), room->second.end(), userId) != room->second.end())
        return false;
    room->second.push_back(userId);
    return true;
}

bool RoomManager::removeMember(int roomId, int userId)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::map<int, std::vector<int> >::iterator room = m_rooms.find(roomId);
    if (room == m_rooms.end())
        return false;
    std::vector<int> &users = room->second;
    std::vector<int>::iterator it = std::find(users.begin(), users.end(), userId);
    if (it == users.end())
        return false;
    users.erase(it);
    if (users.empty())
        m_rooms.erase(room);
    return true;
}

std::vector<int> RoomManager::members(int roomId) const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::map<int, std::vector<int> >::const_iterator room = m_rooms.find(roomId);
    if (room == m_rooms.end())
        return std::vector<int>();
    return room->second;
}

bool RoomManager::empty() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_rooms.empty();
}
