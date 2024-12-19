#include "Database.hpp"

Database::Database(const std::string& db_name)
{
    int exit = sqlite3_open(db_name.c_str(), &_DB);

    if (exit) {
        std::cout << "Error open DB: " << sqlite3_errmsg(_DB) << std::endl;
        _DB = nullptr;
    }
    else {
        std::cout << "Database opened successfully!" << std::endl;
    }
}

Database::~Database()
{
    sqlite3_close(_DB);
}

int Database::createDataBase(const std::string& db_name)
{
    sqlite3* DB;
    std::string sql =   "CREATE TABLE IF NOT EXISTS users ("
                        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "USERNAME TEXT NOT NULL UNIQUE, "
                        "PASSWORD TEXT NOT NULL);";

    int exit_status = sqlite3_open(db_name.c_str(), &DB);

    if (exit_status != SQLITE_OK)
    {
        std::cout << "Error in creating/opening database: " << sqlite3_errmsg(DB) << std::endl;
        return exit_status;
    }
    else {
        char* errorMessage;
        exit_status = sqlite3_exec(DB, sql.c_str(), NULL, 0, &errorMessage);

        if (exit_status != SQLITE_OK) {
            std::cout << "Error creating table: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        }
        sqlite3_close(DB);
    }
    return exit_status;
}

bool Database::addNewUser(const std::string& username, const std::string& password)
{
    std::string sql_query = "INSERT INTO users (USERNAME, PASSWORD) VALUES('" + username + "', '" + password + "')";
    char* errorMessage = nullptr;
    int result = sqlite3_exec(_DB, sql_query.c_str(), nullptr, 0, &errorMessage);

    if (result != SQLITE_OK) {
        std::cout << "Failed to insert data: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);  // Free the error message memory
        return false;
    }

    return true;
}

bool Database::validateUser(const std::string& username, const std::string& password)
{
    std::string sql_query = "SELECT * FROM users WHERE USERNAME = '" + username + "' AND PASSWORD = '" + password + "';";
    sqlite3_stmt* stmt;
    bool valid = false;

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(_DB, sql_query.c_str(), sql_query.size(), &stmt, nullptr) == SQLITE_OK) {
        // Step through the results
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            valid = true;  // If row, the user is valid
        }
    }
    else {
        std::cout << "Failed to validate user: " << sqlite3_errmsg(_DB) << std::endl;
    }

    sqlite3_finalize(stmt);

    return valid;
}

