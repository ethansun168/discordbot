#include "bot.h"
 
// TODO: Use GMP to handle large numbers

std::string BOT_TOKEN;
const char* DB_NAME = "var/users.sqlite3";

std::vector<Item> items;
std::vector<Command> commands;
dpp::embed helpEmbed;
dpp::embed shopEmbed;
dpp::embed noAccountEmbed = dpp::embed()
    .set_color(dpp::colors::red)
    .set_title("Error: No account found!")
    .set_description("Create an account with `/create`")
    .set_timestamp(time(0));

dpp::embed noDatabase = dpp::embed()
    .set_color(dpp::colors::red)
    .set_title("Error: Database connection failed")
    .set_description("Please try again later")
    .set_timestamp(time(0));

long getRate(const std::string& userID) {
    // Requires that userID exists
    const char* sql = R"(
        SELECT SUM(ui.quantity * i.rate)
        FROM userItems ui
        JOIN items i ON ui.item_ID = i.id
        WHERE ui.user_ID = ?
        GROUP BY ui.user_ID;
    )";
    Database db;
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());
    return sqlite3_column_int64(stmt.get(), 0);
}

dpp::embed create(const dpp::user& user) {
    std::string userID = user.id.str();

    if (userExists(userID)) {
        return dpp::embed()
            .set_color(dpp::colors::red)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("Error: Account already exists")
            .set_description("You already have an account")
            .set_timestamp(time(0));
    }

    Database db;
    if (!db.isValid()) {
        return noDatabase;
    }
    const char* createUser = "INSERT INTO users (id, balance, last_updated) VALUES (?, 0, ?);";
    Statement createStmt(db, createUser);
    sqlite3_bind_text(createStmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(createStmt.get(), 2, getTime());
    sqlite3_step(createStmt.get());

    return dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("Account Created")
        .set_description("You have successfully created an account")
        .set_timestamp(time(0));
}

void updateBalance(const std::string& userID) {
    // Requires userID has an account and database is valid
    long currentTime = getTime();
    const char* sql = "SELECT last_updated FROM users WHERE id = ?;";
    long lastUpdated = 0;
    Database db;
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());
    lastUpdated = sqlite3_column_int64(stmt.get(), 0);
    long timePassed = currentTime - lastUpdated;
    long rate = getRate(userID);
    long amount = rate * timePassed;
    const char* updateSql = "UPDATE users SET balance = balance + ?, last_updated = ? WHERE id = ?;";
    Statement updateStmt(db, updateSql);
    sqlite3_bind_int64(updateStmt.get(), 1, amount);
    sqlite3_bind_int64(updateStmt.get(), 2, currentTime);
    sqlite3_bind_text(updateStmt.get(), 3, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(updateStmt.get());
}

long balance(const dpp::user& user) {
    std::string userID = user.id.str();

    if (!userExists(userID)) {
        return -1;
    }
    
    updateBalance(userID);

    const char* sql = "SELECT balance FROM users WHERE id = ?;";
    Database db;
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());
    return sqlite3_column_int64(stmt.get(), 0);
}

dpp::embed work(const dpp::user& user) {
    // Increment balance by 1
    std::string userID = user.id.str();
    if (!userExists(userID)) {
        return noAccountEmbed;
    }
    const char* sql = "UPDATE users SET balance = balance + 1 WHERE id = ?;";
    Database db;
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());
    return dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("You worked!")
        .set_description("Your new balance is: " + formatBalance(balance(user)))
        .set_timestamp(time(0));
}

dpp::embed gamble(const dpp::user& user, const dpp::command_value& amount_value) {
    
    std::string userID = user.id.str();
    if (!userExists(userID)) {
        return noAccountEmbed;
    }

    // amountStr can be "all", "half", or a number
    std::string amountStr = std::get<std::string>(amount_value);

    long userBalance = balance(user);

    long amount = 0;
    if (amountStr == "all") {
        amount = userBalance;
    }
    else if (amountStr == "half") {
        amount = userBalance / 2;
    }
    else {
        try {
            amount = std::stol(amountStr);
        }
        catch (...) {
            return dpp::embed()
                .set_color(dpp::colors::red)
                .set_author(user.username, "", user.get_avatar_url())
                .set_title("Error: Invalid amount")
                .set_description("You gambled an invalid amount")
                .set_timestamp(time(0));
        }
    }

    if (amount < 1) {
        return dpp::embed()
            .set_color(dpp::colors::red)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("Error: Invalid amount")
            .set_description("You gambled an invalid amount")
            .set_timestamp(time(0));
    }

    if (userBalance < amount) {
        return dpp::embed()
            .set_color(dpp::colors::red)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("Error: Insufficient funds")
            .set_description("You don't have enough money to gamble " + formatBalance(amount))
            .set_timestamp(time(0));
    }

    Database db;
    // Take away amount from user
    const char* sql = "UPDATE users SET balance = balance - ? WHERE id = ?;";
    Statement stmt(db, sql);
    sqlite3_bind_int64(stmt.get(), 1, amount);
    sqlite3_bind_text(stmt.get(), 2, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());

    // Generate a random number between 0 and 1
    double random = (double)rand() / RAND_MAX;
    if (random < 0.5) {
        // User lost
        return dpp::embed()
            .set_color(dpp::colors::red)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("You lost!")
            .set_description("You lost " + formatBalance(amount))
            .set_timestamp(time(0));
    }
    else {
        // User won
        const char* sqlWin = "UPDATE users SET balance = balance + ? WHERE id = ?;";
        Statement stmt2(db, sqlWin);
        sqlite3_bind_int64(stmt2.get(), 1, amount * 2);
        sqlite3_bind_text(stmt2.get(), 2, userID.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt2.get());
        return dpp::embed()
            .set_color(dpp::colors::sti_blue)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("You won!")
            .set_description("You won " + formatBalance(amount) + "!")
            .set_timestamp(time(0));
    }
}

std::vector<std::pair<std::string, long>> getLeaderboard(int num) {
    std::vector<std::pair<std::string, long>> lb;
    Database db;
    if (!db.isValid()) {
        return lb;
    }
    const char* sql = "SELECT id, balance FROM users ORDER BY balance DESC LIMIT ?;";

    Statement stmt(db, sql);
    sqlite3_bind_int(stmt.get(), 1, num);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string userID = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        long balance = sqlite3_column_int64(stmt.get(), 1);
        lb.push_back({userID, balance});
    }
    return lb;
}

dpp::embed buy(const dpp::user& user, const std::string& itemStr, const std::string& quantityStr) {
    std::string userID = user.id.str();
    if (!userExists(userID)) {
        return noAccountEmbed;
    }

    dpp::embed invalidItemEmbed = dpp::embed()
        .set_color(dpp::colors::red)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("Error: Invalid item")
        .set_description("Type `/shop` to see the items you can buy");

    int itemID = 0;
    try {
        itemID = std::stoi(itemStr);
    }
    catch (...) {
        return invalidItemEmbed;
    }

    if (itemID < 1 || itemID > items.size()) {
        return invalidItemEmbed;
    }
    --itemID;

    Item item = items[itemID];

    dpp::embed invalidQuantityEmbed = dpp::embed()
        .set_color(dpp::colors::red)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("Error: Invalid quantity")
        .set_description("You entered an invalid quantity")
        .set_timestamp(time(0));

    long quantity = 1;
    try {
        quantity = std::stol(quantityStr);
    }
    catch (...) {
        return invalidQuantityEmbed;
    }

    if (quantity < 1) {
        return invalidQuantityEmbed;
    }

    long userBalance = balance(user);
    long totalCost = item.cost * quantity;
    if (userBalance < totalCost) {
        return dpp::embed()
            .set_color(dpp::colors::red)
            .set_author(user.username, "", user.get_avatar_url())
            .set_title("Error: Insufficient funds")
            .set_description("You don't have enough money to buy **" + item.name + " (" + quantityStr + "x)**")
            .set_timestamp(time(0));
    }

    Database db;
    const char* sql = "UPDATE users SET balance = balance - ? WHERE id = ?;";
    Statement stmt(db, sql);
    sqlite3_bind_int64(stmt.get(), 1, totalCost);
    sqlite3_bind_text(stmt.get(), 2, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt.get());

    const char* sql2 = R"(
        INSERT INTO userItems (user_ID, item_ID, quantity)
        VALUES (?, ?, 1)
        ON CONFLICT(user_ID, item_ID) 
        DO UPDATE SET quantity = quantity + ?;
    )";

    Statement stmt2(db, sql2);
    sqlite3_bind_text(stmt2.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt2.get(), 2, itemID);
    sqlite3_bind_int64(stmt2.get(), 3, quantity);
    sqlite3_step(stmt2.get());

    return dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("You bought an item!")
        .set_description("You bought **" + item.name + " (" + quantityStr + "x)**\nYour new balance is: " + formatBalance(balance(user)) + "\nYour new rate is: " + formatRate(getRate(user.id.str())))
        .set_timestamp(time(0));
}

dpp::embed inventory(const dpp::user& user) {
    std::string userID = user.id.str();
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_author(user.username, "", user.get_avatar_url())
        .set_title("You own:")
        .set_timestamp(time(0));
    
    if (!userExists(userID)) {
        return noAccountEmbed;
    }

    // Get the user's items
    Database db;
    const char* sql = R"(
        SELECT i.name, ui.quantity
        FROM userItems ui
        JOIN items i ON ui.item_ID = i.id
        WHERE ui.user_ID = ?;
    )";
    Statement stmt(db, sql);
    sqlite3_bind_text(stmt.get(), 1, userID.c_str(), -1, SQLITE_STATIC);
    int count = 1;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string itemName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        long quantity = sqlite3_column_int64(stmt.get(), 1);
        embed.add_field(
            "- " + std::to_string(quantity) + "x " + itemName + " [" + std::to_string(count++) + "]",
            ""
        );
    }

    return embed;
}

void handleSlashCommand(const dpp::slashcommand_t& event) {
    std::string commandName = event.command.get_command_name();
    dpp::user user = event.command.get_issuing_user();
    if (commandName == "create") {
        event.reply(create(user));
    }
    else if (commandName == "balance") {
        long bal = balance(user);
        if (bal == -1) {
            event.reply(noAccountEmbed);
            return;
        }

        dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_author(user.username, "", user.get_avatar_url())
        .add_field(
            "Your balance is:",
            formatBalance(bal)
        )
        .add_field(
            "You are gaining:",
            formatRate(getRate(user.id.str()))
        )
        .set_timestamp(time(0));
        
        event.reply(embed);
    }
    else if (commandName == "work") {
        event.reply(work(user));
    }
    else if (commandName == "gamble") {
        event.reply(gamble(user, event.get_parameter("amount")));
    }
    else if (commandName == "leaderboard") {
        int num = 5;
        std::vector<std::pair<std::string, long>> lb = getLeaderboard(num);

        // Create description string
        std::string desc = "";
        for (size_t i = 0; i < lb.size(); ++i) {
            desc += std::to_string(i + 1) + ". <@" + lb[i].first + "> - " + formatBalance(lb[i].second) + "\n";
        }

        dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::sti_blue)
            .set_title("Top " + std::to_string(num) + " Richest Users")
            .set_description(desc)
            .set_timestamp(time(0));
        
        event.reply(embed);
    }
    else if (commandName == "help") {
        event.reply(helpEmbed);
    }
    else if (commandName == "shop") {
        event.reply(shopEmbed);
    }
    else if (commandName == "buy") {
        std::string item = std::get<std::string>(event.get_parameter("item"));
        std::string quantity = "1";
        if (std::holds_alternative<std::string>(event.get_parameter("quantity"))) {
            quantity = std::get<std::string>(event.get_parameter("quantity"));
        }

        event.reply(buy(user, item, quantity));
    }
    else if (commandName == "inventory") {
        event.reply(inventory(user));
    }
}

dpp::embed helpEmb() {
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_title("Help")
        .set_author("The Double L", "", "")
        .set_description("Help page")
        .set_timestamp(time(0));
    for (const Command& command : commands) {
        // If there is an option, add it to the description
        if (command.options.size() > 0) {
            std::string name = "`" + command.name + "` ";
            std::string desc = "";
            for (size_t i = 0; i < command.options.size(); ++i) {
                name += "`[" + command.options[i].name + "`] ";
                desc += "`" + command.options[i].name + "` - " + command.options[i].description + "\n";
            }
            embed.add_field(
                name,
                desc
            );
            continue;
        }
        embed.add_field(
            "`" + command.name + "`",
            command.description
        );
    }
    return embed;
}

dpp::embed shopEmb() {
    dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_title("Shop")
        .set_description("Shop page")
        .set_timestamp(time(0));
    for (int i = 0; i < items.size(); ++i) {
        embed.add_field(
            "[" + std::to_string(i + 1) + "] " + items[i].name,
            "Cost: " + formatBalance(items[i].cost) + "\nRate: " + formatRate(items[i].rate),
            true
        );
    }
    return embed;
}

void insertItems(const std::vector<Item>& items) {
    Database db;
    if (!db.isValid()) {
        return;
    }
    const char* sql = "INSERT INTO items (id, name, price, rate) VALUES (?, ?, ?, ?);";
    Statement stmt(db, sql);
    for (int i = 0; i < items.size(); ++i) {
        sqlite3_bind_int(stmt.get(), 1, i);
        sqlite3_bind_text(stmt.get(), 2, items[i].name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt.get(), 3, items[i].cost);
        sqlite3_bind_int(stmt.get(), 4, items[i].rate);
        sqlite3_step(stmt.get());
        sqlite3_reset(stmt.get());
    }
}

/*
    ./bot y/n commands.in items.in
    y/n (1) - whether to register commands
*/
int main(int argc, char* argv[]) {
    std::ifstream file("apikey");
    file >> BOT_TOKEN;
    file.close();

    // Read input files
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " y/n commands.in items.in" << std::endl;
        return 1;
    }
    commands = parseCommands(argv[2]);
    helpEmbed = helpEmb();

    items = parseItems(argv[3]);
    shopEmbed = shopEmb();
    insertItems(items);

    dpp::cluster bot(BOT_TOKEN);
 
    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand(handleSlashCommand);
 
    bot.on_ready([&bot, &argv](const dpp::ready_t& event) {
        if (argv[1][0] == 'y' && dpp::run_once<struct register_bot_commands>()) {
            std::cout << "Registering commands..." << std::endl;
            std::vector<dpp::slashcommand> com;
            for (const Command& command : commands) {
                dpp::slashcommand slash_command(command.name, command.description, bot.me.id);
                for (size_t i = 0; i < command.options.size(); ++i) {
                    slash_command.add_option(
                        dpp::command_option(
                            dpp::co_string,
                            command.options[i].name,
                            command.options[i].description,
                            command.options[i].required
                        )
                    );
                }
                com.push_back(slash_command);
            }
            // bot.global_bulk_command_delete();
            bot.guild_bulk_command_create(com, 1344149950291775550);
        }
    });
 
    bot.start(dpp::st_wait);
}