#include "Server.hpp"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string Server::sendRequestToAI(const std::string& prompt) {
    CURL* curl;
    CURLcode api_status_code;
    std::string response_data;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + API_TOKEN).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
       

        std::string json_data = "{\"inputs\": \"" + prompt + "\", \"parameters\": {\"max_new_tokens\": " + std::to_string(MAX_TOKENS) + "}}";

        curl_easy_setopt(curl, CURLOPT_URL, API_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        api_status_code = curl_easy_perform(curl);

        if (api_status_code != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(api_status_code) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    else {
        std::cerr << "Failed to initialize curl" << std::endl;
    }

    return response_data;
}

Server::Server(boost::asio::io_service& io_service)
    : _acceptor(io_service), _socket(io_service) {
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), PORT);
    _acceptor.open(endpoint.protocol());
    _acceptor.bind(endpoint);
    _acceptor.listen();
    acceptConnections();
}

Server::~Server()
{
    _acceptor.close();
    for (auto& client : _clients)
    {
        if (client.second.socket->is_open())
        {
            client.second.socket->close();
        }
    }
    _clients.clear();

}

void Server::acceptConnections()
{
    auto new_socketptr = std::make_shared<boost::asio::ip::tcp::socket>(_acceptor.get_executor());
    _acceptor.async_accept(*new_socketptr, [this, new_socketptr](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "New connection accepted.\n";
            uint32_t socket_fd = new_socketptr->native_handle();
            _clients[socket_fd].socket = new_socketptr;
            acceptConnections();
            readingInitConnection(socket_fd); //reading login or sign in message
            startReading(socket_fd);
        }
        else {
            throw std::runtime_error("Error accepting connection: " + error.message());
        }
        });
}

void Server::readingInitConnection(uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    std::vector<char> buffer(REQUEST_BUFFER_SIZE);
    boost::system::error_code error;
    // Perform synchronous receive operation
    size_t bytes = socketptr.socket->read_some(boost::asio::buffer(buffer), error);

    if (error) {
        throw std::runtime_error("Error receiving data: " + error.message());
    }
    // Convert the received data to a string and process it
    std::string message(buffer.data(), bytes);
    handleClientConnection(message, socketfd);
    redingRoomConnection(socketfd); //reding create or join message
}


void Server::handleClientConnection(const std::string& message, uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    std::stringstream ss(message);
    std::string response;
    std::string action, username, password;

    std::getline(ss, action, ':');
    std::getline(ss, username, ':');
    std::getline(ss, password, ':');

    Database db("myDB.db");

    if (action == "login")
    {
        if (db.validateUser(username, password))
        {
            std::cout << username << " Sign In successfully." << std::endl;
            response = "1"; //login successfully
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            _clients[socketfd].username = username;
        }
        else
        {   
            response = "0"; //password or username is not correct
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            readingInitConnection(socketfd);
        }
    }
    else if (action == "signup")
    {
        if (db.addNewUser(username, password))
        {
            std::cout << "User created: " << username << std::endl;
            response = "1";//signUp successfully
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            _clients[socketfd].username = username;
        }
        else
        {
            response = "2"; //username is already exists
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            readingInitConnection(socketfd);
        }
        
    }
}

void Server::redingRoomConnection(uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    std::vector<char> buffer(REQUEST_BUFFER_SIZE);
    boost::system::error_code error;
    // Perform synchronous receive operation
    size_t bytes = socketptr.socket->read_some(boost::asio::buffer(buffer), error);
    if (error) {
        throw std::runtime_error("Error receiving data: " + error.message());
    }
    // Convert the received data to a string and process it
    std::string message(buffer.data(), bytes);
    handleRoomConnection(message, socketfd);
}

void Server::handleRoomConnection(const std::string& message, uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    std::stringstream ss(message);
    std::string action, room_key, response;

    std::getline(ss, action, ':');
    std::getline(ss, room_key, ':');
    
    if (action == "create")
    {
        if (_rooms.find(room_key) == _rooms.end())
        {
            _rooms[room_key] = Room(room_key);
            _rooms[room_key].addClient(socketptr.socket);
            _client_to_room[socketptr.socket->native_handle()] = room_key;
            response = '1'; //create succesfully
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            std::cout << room_key << " Is Created\n";
        }
        else
        {
            response = '0'; //this key is in use
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            redingRoomConnection(socketfd);
        }
    }
    else if (action == "join")
    {
        if (_rooms.find(room_key) == _rooms.end())
        {
            response = '0'; //key is not exist
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
            redingRoomConnection(socketfd);
        }
        else
        {
            _rooms[room_key].addClient(socketptr.socket);
            _client_to_room[socketptr.socket->native_handle()] = room_key;
            response = '1'; //join succesfully
            boost::asio::write(*socketptr.socket, boost::asio::buffer(response));
        }
    }
}

void Server::broadcastToRoom(const std::string& message, const std::string& room_key,uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    _rooms[room_key].broadcast(message, socketptr.socket);
}

void Server::handleClientDisconnect(uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];

    std::string room_key = _client_to_room[socketfd];
    _rooms[room_key].removeClient(socketptr.socket);
    _client_to_room.erase(socketfd);
    _clients.erase(socketfd);
    socketptr.socket->close();
    std::cout << socketptr.username << " Is disconnected\n";
}

void Server::startReading(uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    auto buffer = std::make_shared<std::vector<char>>(REQUEST_BUFFER_SIZE);
    socketptr.socket->async_receive(boost::asio::buffer(*buffer), [this, socketfd, buffer](const boost::system::error_code& error, size_t bytes)
        {
            if (!error) 
            {
                handleReadCallBack(error, bytes, socketfd, buffer);
            }
            else {
                handleClientDisconnect(socketfd);
            }
        });
}

void Server::handleReadCallBack(const boost::system::error_code& error, std::size_t bytes, uint32_t socketfd, std::shared_ptr<std::vector<char>> buffer)
{
    auto socketptr = _clients[socketfd];
    if (!error) {
        std::string message(buffer->data(), bytes);

        if (message == "$home$")
        {
            disconnectFromRoom(socketfd);
            readingInitConnection(socketfd);
        }
        else if(message == "$room$")
        {
            disconnectFromRoom(socketfd);
            redingRoomConnection(socketfd);
        }
        else if (message[0] == '!')
        {
            broadcastAiResponse(message, socketfd);
        }
        else
        {
            std::string room_key = _client_to_room[socketfd]; //find the room the client is in
            broadcastToRoom(message, room_key, socketfd);
        }
        startReading(socketfd);
    }
    else {
        throw std::runtime_error("Error in reading: " + error.message());
    }
}

void Server::disconnectFromRoom(uint32_t socketfd)
{
    auto socketptr = _clients[socketfd];
    std::string room_key = _client_to_room[socketfd];
    _rooms[room_key].removeClient(socketptr.socket);
    _client_to_room.erase(socketfd);                        
}

void Server::broadcastAiResponse(const std::string& message, uint32_t socketfd)
{
    //broadcast the message
    std::string room_key = _client_to_room[socketfd];
    broadcastToRoom(message, room_key, socketfd);

    // Send the client's message to the AI and get the response
    std::string ai_response = sendRequestToAI(message.substr(1));
    auto message_ptr = std::make_shared<std::string>(ai_response);
    // Send the AI response back to the same client
    auto socketptr = _clients[socketfd];
    boost::asio::async_write(*socketptr.socket, boost::asio::buffer(*message_ptr), [this, message_ptr, socketfd,room_key](const boost::system::error_code& error, size_t bytes)
        {
            if (!error) {
                broadcastToRoom(*message_ptr, room_key, socketfd);
            }
            else {
                throw std::runtime_error("Error writing data: " + error.message());
            }
        });
}
