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
        if (client.second->is_open())
        {
            client.second->close();
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
            int socket_fd = new_socketptr->native_handle();
            _clients[socket_fd] = new_socketptr;
            readingInitConnection(new_socketptr); //reading login or sign in message
            redingRoomConnection(new_socketptr); //reding create or join message
            startReading(new_socketptr);
            acceptConnections();
        }
        else {
            throw std::runtime_error("Error accepting connection: " + error.message());
        }
        });
}

void Server::readingInitConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    std::vector<char> buffer(REQUEST_BUFFER_SIZE);
    boost::system::error_code error;
    // Perform synchronous receive operation
    size_t bytes = socket->read_some(boost::asio::buffer(buffer), error);

    if (error) {
        throw std::runtime_error("Error receiving data: " + error.message());
    }
    // Convert the received data to a string and process it
    std::string message(buffer.data(), bytes);
    handleClientConnection(message, socket);
}


void Server::handleClientConnection(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
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
            response = "1";
            boost::asio::write(*socket, boost::asio::buffer(response));
        }
        else
        {   
            response = "0"; //password or username is not correct
            boost::asio::write(*socket, boost::asio::buffer(response));
            readingInitConnection(socket);
        }
    }
    else if (action == "signup")
    {
        if (db.addNewUser(username, password))
        {
            std::cout << "User created: " << username << std::endl;
            response = "1";
            boost::asio::write(*socket, boost::asio::buffer(response));
        }
        else
        {
            response = "2"; //username is already exists
            boost::asio::write(*socket, boost::asio::buffer(response));
            readingInitConnection(socket);
        }
        
    }
}

void Server::redingRoomConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    std::vector<char> buffer(REQUEST_BUFFER_SIZE);
    boost::system::error_code error;
    // Perform synchronous receive operation
    size_t bytes = socket->read_some(boost::asio::buffer(buffer), error);
    if (error) {
        throw std::runtime_error("Error receiving data: " + error.message());
    }
    // Convert the received data to a string and process it
    std::string message(buffer.data(), bytes);
    handleRoomConnection(message, socket);
}

void Server::handleRoomConnection(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    std::stringstream ss(message);
    std::string action, room_key, response;

    std::getline(ss, action, ':');
    std::getline(ss, room_key, ':');
    
    if (action == "create")
    {
        if (_rooms.find(room_key) == _rooms.end())
        {
            _rooms[room_key] = Room(room_key);
        }
        else
        {
            response = '0'; //this key is in use
            boost::asio::write(*socket, boost::asio::buffer(response));
            redingRoomConnection(socket);
        }
    }
    else if (action == "join")
    {
        if (_rooms.find(room_key) == _rooms.end())
        {
            response = '1'; //key is not exist
            boost::asio::write(*socket, boost::asio::buffer(response));
            redingRoomConnection(socket);
        }
        else
        {
            _rooms[room_key].addClient(socket);
            _client_to_room[socket->native_handle()] = room_key;
        }
    }
}

void Server::broadcastToRoom(const std::string& message, const std::string& room_key,int socketfd)
{
    auto socketptr = _clients[socketfd];
    _rooms[room_key].broadcast(message, socketptr);
}

void Server::handleClientDisconnect(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    int socketfd = socket->native_handle();

    std::string room_key = _client_to_room[socketfd];
    _rooms[room_key].removeClient(socket);
    _client_to_room.erase(socketfd);
    _clients.erase(socketfd);
    socket->close();
    std::cout << socketfd << " Is disconnected\n";
}

void Server::startReading(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    auto buffer = std::make_shared<std::vector<char>>(REQUEST_BUFFER_SIZE);
    socket->async_receive(boost::asio::buffer(*buffer), [this, socket, buffer](const boost::system::error_code& error, size_t bytes)
        {
            if (!error) 
            {
                handleReadCallBack(error, bytes, socket, buffer);
            }
            else {
                handleClientDisconnect(socket);
            }
        });
}

void Server::handleReadCallBack(const boost::system::error_code& error, std::size_t bytes, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::shared_ptr<std::vector<char>> buffer)
{
    if (!error) {
        std::string message(buffer->data(), bytes);

        if (message == "$home$")
        {
            readingInitConnection(socket);
        }
        else if(message == "$room$")
        {
            redingRoomConnection(socket);
        }
        else if (message[0] == '!')
        {
            
            // Send the client's message to the AI and get the response
            std::string ai_response = sendRequestToAI(message.substr(1));
            auto message_ptr = std::make_shared<std::string>(ai_response);

            // Send the AI response back to the same client
            boost::asio::async_write(*socket, boost::asio::buffer(*message_ptr), [this, message_ptr](const boost::system::error_code& error, size_t bytes)
                {
                    handleWriteCallBack(error);
                });
        }
        else
        {
            uint32_t socketfd = socket->native_handle();
            std::string room_key = _client_to_room[socketfd]; //find the room the client is in
            broadcastToRoom(message, room_key, socketfd);

        }
        startReading(socket);
    }
    else {
        throw std::runtime_error("Error in reading: " + error.message());
    }
}

void Server::handleWriteCallBack(const boost::system::error_code& error)
{
    if (error) {
        throw std::runtime_error("Error writing data: " + error.message());
    }
}
