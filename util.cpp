#include "bot.h"
#include <chrono>

long getTime() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string formatBalance(long balance) {
    // Add commas to balance
    std::string num_str = std::to_string(balance);
    int insert_position = num_str.length() - 3;

    while (insert_position > 0) {
        num_str.insert(insert_position, ",");
        insert_position -= 3;
    }

    return "`$" + num_str + "`";
}

std::string formatRate(int rate) {
    std::string r = formatBalance(rate);
    // Take out first and last character
    r = r.substr(1, r.length() - 2);
    return "`+" + r + "/s`";
}

bool userExists(const std::string& userID) {
    Database db;
    if (!db.isValid()) {
        return false;
    }
    const char* sql = "SELECT * FROM users WHERE id = ?;";
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    return sqlite3_step(stmt.get()) == SQLITE_ROW;
}
