#pragma once
#include <fstream>
#include <dpp/dpp.h>
#include <sqlite3.h>
#include <iostream>
#include <future>
#include <utility>
#include <vector>
#include <string>

extern const char* DB_NAME;

struct Option {
    std::string name;
    std::string description;
    bool required;
};

struct Command {
    std::string name;
    std::string description;  
    std::vector<Option> options;
};

struct Item {
    std::string name;
    int cost;
    int rate;
};

// sqlite3 RAII
class Database {
private:
    sqlite3* db;

public:
    Database() {
        if (sqlite3_open(DB_NAME, &db)) {
            std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
            db = nullptr;
        }
    }

    bool isValid() const {
        return db;
    }

    sqlite3* get() const {
        return db;
    }

    ~Database() {
        if (db) {
            sqlite3_close(db);
        }
    }
};

// sqlite3_stmt RAII
class Statement {
private:
    sqlite3_stmt* stmt;
public:
    Statement(const Database& db, const char* sql) {
        if (sqlite3_prepare_v2(db.get(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db.get()) << std::endl;
            stmt = nullptr;
        }
    }

    ~Statement() {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }

    bool isValid() const {
        return stmt;
    }

    sqlite3_stmt* get() const {
        return stmt;
    }
};

std::string formatBalance(long balance);
std::string formatRate(int rate);
bool userExists(const std::string& userID);
std::vector<std::string> split(const std::string& str, char delim);
std::vector<Command> parseCommands(const char* filename);
std::vector<Item> parseItems(const char* filename);
long getTime();