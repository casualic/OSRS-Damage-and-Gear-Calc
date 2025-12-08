#include "monster.h"
#include <iostream>
#include "external/simdjson.h"


Monster::Monster(std::string n) : name_(std::move(n)) {}

/*
void Monster::fetchStats(){
    simdjson::ondemand::parser parser;
    simdjson::padded_string json = simdjson::padded_string::load("monsters-nodrops.json");

    simdjson::ondemand::document doc = parser.iterate(json);

    // Your JSON is a map: "1": {...}, "2": {...}
    // We iterate over the fields of the main object

    
    for (auto field : doc.get_object()) {
        std::string_view key = field.unescaped_key();
        simdjson::ondemand::value value = field.value();
        std::string_view name;

        simdjson::ondemand::object obj = value.get_object();
        if (obj["name"].get(name) == simdjson::SUCCESS && name == name_){
            int64_t hp = value["hitpoints"];
            std::cout << "Found " + name_ + " ! HP: " << hp << std::endl;

                                // ...existing code...
                // simdjson::ondemand::object obj = value.get_object();
        
            for (auto field : obj) {
                // get key as string_view from simdjson_result, then copy
                std::string_view key_sv;
                if (field.key().get(key_sv) != simdjson::SUCCESS) continue;
                std::string key(key_sv);
        
                auto v = field.value();
        
                int64_t i;            if (v.get(i) == simdjson::SUCCESS) { stats_[key] = static_cast<int>(i); continue; }
                double d;             if (v.get(d) == simdjson::SUCCESS) { stats_[key] = d;                  continue; }
                bool b;               if (v.get(b) == simdjson::SUCCESS) { stats_[key] = b;                  continue; }
                std::string_view s;   if (v.get(s) == simdjson::SUCCESS) { stats_[key] = std::string(s);     continue; }
                // skip objects/arrays/null
            }
            break;
                
        }
    }
};
*/

void Monster::loadFromJSON(const std::string &filepath){};