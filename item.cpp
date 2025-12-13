// item.cpp
#include "item.h"
#include <iostream>
#include <fstream>

using json = nlohmann::json;

Item::Item(std::string n) : name_(std::move(n)), id_(-1), price_(0) {}
Item::Item(int id) : id_(id), name_(""), price_(0) {}

void Item::parseItemJSON(const json& item) {
    if (item.contains("equipment")) {
        auto itemStats = item["equipment"];
        for (auto& [key, value] : itemStats.items()) {
            if (value.is_number_integer()) {
                stats_int_[key] = value.get<int>();
            } else if (value.is_string()) {
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()) {
                stats_bool_[key] = value.get<bool>();
            }
        }
    }
    if (item.contains("weapon")) {
        auto itemDetails = item["weapon"];
        for (auto& [key, value] : itemDetails.items()) {
            if (value.is_number_integer()) {
                stats_int_[key] = value.get<int>();
            } else if (value.is_string()) {
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()) {
                stats_bool_[key] = value.get<bool>();
            }
        }
    }
}

void Item::fetchStats(int id, const json& allItems) {
    std::string id_str = std::to_string(id);
    if (allItems.contains(id_str)) {
        auto item = allItems[id_str];
        name_ = item.value("name", name_);
        id_ = id;
        parseItemJSON(item);
    }
}

void Item::fetchStats(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Could not open item file: " << filepath << "\n";
        return;
    }
    
    json items;
    try {
        items = json::parse(file);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing item JSON: " << e.what() << "\n";
        return;
    }

    if (id_ != -1) {
        std::string id_str = std::to_string(id_);
        if (items.contains(id_str)) {
            auto item = items[id_str];
            name_ = item.value("name", name_);
            parseItemJSON(item);
            return;
        }
    }

    for (auto& [key, item] : items.items()) {
        if (item.contains("name") && item["name"] == name_) {
            try {
                id_ = std::stoi(key);
            } catch (...) {}
            parseItemJSON(item);
            break;
        }
    }
}

#ifndef __EMSCRIPTEN__
int Item::fetchPrice() {
    std::cout << "Price fetching via API disabled. Please ensure latest_prices.json is present.\n";
    return 0;
}
#endif
