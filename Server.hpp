#pragma once

#include "Database.hpp"
#include "Room.hpp"
#include <boost/asio.hpp>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

#define CURL_STATICLIB
#include <curl/curl.h>


class Server {

private:
    
    const std::string API_URL = "https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.2";
    const std::string API_TOKEN = "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"; // Hugging Face API token
    static constexpr auto MAX_TOKENS = 1000;
    static constexpr auto PORT = 8000;
    static constexpr auto REQUEST_BUFFER_SIZE = 1024;
    boost::asio::ip::tcp::acceptor _acceptor;
    boost::asio::ip::tcp::socket _socket;
    std::unordered_map<int, std::shared_ptr<boost::asio::ip::tcp::socket>> _clients;

    std::unordered_map<std::string, Room> _rooms;

    std::unordered_map<int, std::string> _client_to_room;

    //Send the client request to the AI API by a HTTP request
    std::string sendRequestToAI(const std::string& prompt);

    // Accept incoming connections from clients
    void acceptConnections();

    //gets Sign Up or Login message from the client
    void readingInitConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    //handle the Sign Up or Login message
    void handleClientConnection(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void redingRoomConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void handleRoomConnection(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void broadcastToRoom(const std::string& message, const std::string& room_key, int socketfd);

    void handleClientDisconnect(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    // Start reading data from a specific client socket
    void startReading(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    // Handle the callback when data is read from a client socket
    void handleReadCallBack(const boost::system::error_code& error, std::size_t bytes, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::shared_ptr<std::vector<char>> buffer);

    // Handle the callback when data is written to a client socket
    void handleWriteCallBack(const boost::system::error_code& error);
public:

    Server(boost::asio::io_service& io_service);
    ~Server();


};