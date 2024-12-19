#pragma once

#include "sqlite/sqlite3.h"
#include <string>
#include <iostream>

class Database
{
private:
	sqlite3* _DB;

public:
	Database(const std::string& db_name);
	~Database();
	static int createDataBase(const std::string& db_name);
	bool addNewUser(const std::string& username, const std::string& password);
	bool validateUser(const std::string& username, const std::string& password);


};

