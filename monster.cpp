#include "monster.h"
#include <iostream>
#include "external/simdjson.h"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;


Monster::Monster(std::string n) : name_(std::move(n)) {};

void Monster::loadFromJSON(const std::string &filepath){
    std::ifstream file(filepath);
    json monsters = json::parse(file);

    for (auto& [id,monster] : monsters.items()){
        if (monster["name"] == name_){
            for (auto& [key, value] : monster.items()){
                if (value.is_number_integer()){
                    stats_int_[key] = value;
                } else if (value.is_string()){
                    stats_str_[key] = value;
                } else if (value.is_boolean()){
                    stats_bool_[key] = value;
                }
            }
            break;
        }
    }
}