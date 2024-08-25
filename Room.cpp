#include "Room.hpp"

Room::Room(const std::string key)
{
	_room_key = key;
}

Room::~Room()
{
	_clients.clear();
}

void Room::addClient(std::shared_ptr<boost::asio::ip::tcp::socket> client)
{
	_clients.insert(client);
}

void Room::removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> client)
{
	_clients.erase(client);
}

void Room::broadcast(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> sender_socket)
{
	for (auto& client : _clients)
	{
		if (client != sender_socket)
		{
			auto message_ptr = std::make_shared<std::string>(message);
			boost::asio::async_write(*client, boost::asio::buffer(*message_ptr),
				[message_ptr](const boost::system::error_code& error, size_t bytes) {
					if (error) {
						throw std::runtime_error("Error broadcasting to client: " + error.message());
					}
				});
		}
	}
}
