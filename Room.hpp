#pragma once

#include <unordered_set>
#include <memory>
#include <string>
#include <boost/asio.hpp>

class Room
{
private:
    std::unordered_set<std::shared_ptr<boost::asio::ip::tcp::socket>> _clients;
    std::string _room_key;

public:
    Room() = default;
    Room(const std::string key);
    ~Room();
    void addClient(std::shared_ptr<boost::asio::ip::tcp::socket> client);
    void removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> client);
    void broadcast(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> sender_socket);

};


