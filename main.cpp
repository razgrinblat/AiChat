#include <iostream>
#include <boost/asio.hpp>
#include "Server.hpp"

int main() 
{
    //create the database
    std::string db_name = "myDB.db";

    int db_status = Database::createDataBase(db_name);
    if (db_status != SQLITE_OK) 
    {
        std::cout << "Failed to create the database. Exiting..." << std::endl;
        return db_status;
    }

    try {
        boost::asio::io_service io_service;
        Server server(io_service);

        std::cout << "Server is running..." << std::endl;

        // Run the io_service to start the asynchronous operations
        io_service.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
