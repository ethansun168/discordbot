#include "bot.h"

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<Command> parseCommands(const char* filename) {
    std::vector<Command> commands;
    std::ifstream file(filename); 
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split(line, '|');
        if (tokens.size() < 2) {
            continue;
        }
        if ((tokens.size() - 2) % 3 != 0) {
            std::cerr << "Invalid format in line: " << line << std::endl;
            continue;
        }
        Command command;
        command.name = tokens[0];
        command.description = tokens[1];
        for (size_t i = 2; i < tokens.size(); i += 3) {
            Option option;
            option.name = tokens[i];
            option.description = tokens[i + 1];
            if (tokens[i + 2] == "true") {
                option.required = true;
            } else if (tokens[i + 2] == "false") {
                option.required = false;
            } else {
                std::cerr << "Invalid required flag in line: " << line << std::endl;
                continue;  // Skip this option
            }
            command.options.push_back(option);
        }
        commands.push_back(command);
    }
    file.close();
    return commands;
}

std::vector<Item> parseItems(const char* filename) {
    std::vector<Item> items;
    std::ifstream file (filename);

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split(line, '|');
        if (tokens.size() < 3) {
            continue;
        }
        Item item;
        item.name = tokens[0];
        item.cost = std::stoi(tokens[1]);
        item.rate = std::stoi(tokens[2]);
        items.push_back(item);
    }    

    file.close();
    return items;
}