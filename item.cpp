#include "item.h"
#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

Item::Item(std::string n) : name_(std::move(n)) {};


int Item::fetchPrice(){

    httplib::SSLClient cli("prices.runescape.wiki",443);
    cli.set_follow_location(true);
    cli.set_read_timeout(10, 0);
    cli.set_write_timeout(10, 0);
    cli.set_default_headers({
        {"User-Agent", "OSRSCalc/1.0"},
        {"Accept", "application/json"}
    });
    cli.enable_server_certificate_verification(false); // TEMP: debug only

    // Get mappings first
    auto res_mapping = cli.Get("/api/v1/osrs/mapping");

    if (!res_mapping) {
        std::cerr << "HTTP error: " << static_cast<int>(res_mapping.error()) << "\n";
        return 0;
    }
    if (res_mapping->status != 200) {
        std::cerr << "HTTP status: " << res_mapping->status << "\n";
        return 0;
    }
    
    json mapping = json::parse(res_mapping->body);
    int target_id { };
    for (const auto& entry : mapping) {           // JSON array of objects
        if (!entry.is_object()) continue;
        std::string name = entry.value("name", "");
        int id = entry.value("id", 0);
        // std::cout << name << " (" << id << ")\n";
        if (name == name_){
            target_id = entry.value("id",0);
            break;
        }
    }

    std::cout << "Found Target : " << name_ <<  "|  Target ID:  " << target_id ;

    // Get Latest price 

    auto res_prices = cli.Get("/api/v1/osrs/latest");
    json res_prices_mapping = json::parse(res_prices ->body);
    const json& latest_data = res_prices_mapping.at("data"); // this drops the 'data' wrapper level


    const std::string key = std::to_string(target_id);
    auto it = latest_data.find(key);
    int mid { };
    if (it != latest_data.end()) {
        const json& entry = it.value();
        int high = entry.value("high", 0);
        int low  = entry.value("low", 0);
        // use high/low...
        
        mid = (high + low) / 2;
        std::cout << "Found mid price : "<< mid << std::endl;
        price_ = mid;
    } else {
        std::cerr << "ID " << key << " not found in latest data\n";
    };
    
    std::ofstream json_out("/Users/mateuszdelpercio/Code/C/OSRSCalc/latest_prices.json");
    json_out << res_prices_mapping.dump(2);

    return mid;
    
}