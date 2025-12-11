#include "item.h"
#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

Item::Item(std::string n) : name_(std::move(n)), id_(-1) {};
Item::Item(int id) : id_(id), name_("") {};


int Item::fetchPrice(){
    // Existing implementation (keeping it as is for now, it's fine)
    httplib::SSLClient cli("prices.runescape.wiki",443);
    cli.set_follow_location(true);
    cli.set_read_timeout(10, 0);
    cli.set_write_timeout(10, 0);
    cli.set_default_headers({
        {"User-Agent", "OSRSCalc/1.0"},
        {"Accept", "application/json"}
    });
    cli.enable_server_certificate_verification(false);

    auto res_mapping = cli.Get("/api/v1/osrs/mapping");
    if (!res_mapping || res_mapping->status != 200) return 0;
    
    json mapping = json::parse(res_mapping->body);
    int target_id { id_ };

    if (target_id == -1) {
        for (const auto& entry : mapping) {
            if (!entry.is_object()) continue;
            std::string name = entry.value("name", "");
            if (name == name_){
                target_id = entry.value("id",0);
                id_ = target_id;
                break;
            }
        }
    }
    
    // ... (rest of price fetching) ...
    // Shortened for brevity since I'm overwriting the file.
    // I need to include the FULL content because Write overwrites.
    
    // Re-implementing full fetchPrice for safety
    if (target_id != -1 && name_.empty()) {
         for (const auto& entry : mapping) {
            if (!entry.is_object()) continue;
            if (entry.value("id", 0) == target_id) {
                name_ = entry.value("name", "");
                break;
            }
         }
    }

    std::cout << "Found Target : " << name_ <<  "|  Target ID:  " << target_id << std::endl;

    auto res_prices = cli.Get("/api/v1/osrs/latest");
    json res_prices_mapping = json::parse(res_prices ->body);
    const json& latest_data = res_prices_mapping.at("data");

    const std::string key = std::to_string(target_id);
    auto it = latest_data.find(key);
    int mid { 0 };
    if (it != latest_data.end()) {
        const json& entry = it.value();
        int high = entry.value("high", 0);
        int low  = entry.value("low", 0);
        mid = (high + low) / 2;
        std::cout << "Found mid price : "<< mid << std::endl;
        price_ = mid;
    } else {
        std::cerr << "ID " << key << " not found in latest data\n";
    };
    
    std::ofstream json_out("latest_prices.json");
    json_out << res_prices_mapping.dump(2);

    return mid;
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
