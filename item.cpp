#include "item.h"
#include <iostream>

// #define CPPHTTPLIB_OPENSSL_SUPPORT
// #include "cpp-httplib/httplib.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

Item::Item(std::string n) : name_(std::move(n)), id_(-1) {};
Item::Item(int id) : id_(id), name_("") {};


int Item::fetchPrice(){
    std::cout << "Price fetching via API disabled. Please ensure latest_prices.json is present.\n";
    return 0;
}

void Item::parseItemJSON(const json& item) {
    if (item.contains("equipment")) {
        auto itemStats = item["equipment"];
        for (auto& [key,value] : itemStats.items()){
            if (value.is_number_integer()){
                stats_int_[key] = value.get<int>();
            } else if (value.is_string()){
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()){
                stats_bool_[key] = value.get<bool>();
            }
        }
    }
    if (item.contains("weapon")) {
        auto itemDetails = item["weapon"];
        for (auto& [key,value]: itemDetails.items()){
            if (value.is_number_integer()){
                stats_int_[key] = value.get<int>();
            } else if (value.is_string()){
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()){
                stats_bool_[key] = value.get<bool>();
            }
        }
    }
}

void Item::fetchStats(int id, const json& allItems) {
    std::string id_str = std::to_string(id);
    if (allItems.contains(id_str)) {
        auto item = allItems[id_str];
        name_ = item.value("name", name_); // Update name if missing
        parseItemJSON(item);
    }
}

void Item::fetchStats(const std::string &filepath){
    std::ifstream file(filepath);
    json items = json::parse(file);

    // Optimized ID lookup
    if (id_ != -1) {
        std::string id_str = std::to_string(id_);
        if (items.contains(id_str)) {
            auto item = items[id_str];
            name_ = item.value("name", name_);
            parseItemJSON(item);
            return;
        }
    }

    // Fallback to name search
    for (auto& [key, item] : items.items()){
        if (item["name"] == name_){
             try {
                id_ = std::stoi(key);
             } catch (...) {}
            parseItemJSON(item);
            break;
        }
    }
}
