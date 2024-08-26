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
    struct Client
    {
        std::string username;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket;
    };

    std::unordered_map<uint32_t, Client> _clients;
    
    std::unordered_map<std::string, Room> _rooms;

    std::unordered_map<uint32_t, std::string> _client_to_room;

    //Send the client request to the AI API by a HTTP request
    std::string sendRequestToAI(const std::string& prompt);

    // Accept incoming connections from clients
    void acceptConnections();

    //gets Sign Up or Login message from the client
    void readingInitConnection(uint32_t socketfd);

    //handle the Sign Up or Login message
    void handleClientConnection(const std::string& message, uint32_t socketfd);

    void redingRoomConnection(uint32_t socketfd);

    void handleRoomConnection(const std::string& message, uint32_t socketfd);

    void broadcastToRoom(const std::string& message, const std::string& room_key, uint32_t socketfd);

    void handleClientDisconnect(uint32_t socketfd);

    // Start reading data from a specific client socket
    void startReading(uint32_t socketfd);

    // Handle the callback when data is read from a client socket
    void handleReadCallBack(const boost::system::error_code& error, std::size_t bytes, uint32_t socketfd, std::shared_ptr<std::vector<char>> buffer);

    void disconnectFromRoom(uint32_t socketfd);

    // Handle the callback when data is written to a client socket
    void broadcastAiResponse(const std::string& message, uint32_t socketfd);
public:

    Server(boost::asio::io_service& io_service);
    ~Server();


};