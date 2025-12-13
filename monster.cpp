// monster.cpp
#include "monster.h"
#include <iostream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

Monster::Monster(std::string n) : name_(std::move(n)) {}

void Monster::setInt(const std::string& key, int value) {
    stats_int_[key] = value;
    if (key == "hitpoints") {
        current_hp_ = value;
    }
    if (key == "size") {
        size_ = value;
    }
}

int Monster::getInt(const std::string& key) const {
    auto it = stats_int_.find(key);
    return (it != stats_int_.end()) ? it->second : 0;
}

std::string Monster::getStr(const std::string& key) const {
    auto it = stats_str_.find(key);
    return (it != stats_str_.end()) ? it->second : "";
}

bool Monster::getBool(const std::string& key) const {
    auto it = stats_bool_.find(key);
    return (it != stats_bool_.end()) ? it->second : false;
}

void Monster::resetHP() {
    current_hp_ = stats_int_.count("hitpoints") ? stats_int_.at("hitpoints") : 0;
}

bool Monster::hasAttribute(const std::string& attr) const {
    for (const auto& a : attributes_) {
        if (a == attr) return true;
    }
    return false;
}

void Monster::loadFromJSON(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Could not open monster file: " << filepath << "\n";
        return;
    }
    
    json root;
    try {
        root = json::parse(file);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing monster JSON " << filepath << ": " << e.what() << "\n";
        return;
    }

    auto loadStats = [&](const json& monster) {
        for (auto& [key, value] : monster.items()) {
            if (value.is_number_integer()) {
                int v = value.get<int>();
                stats_int_[key] = v;
                if (key == "hitpoints") {
                    current_hp_ = v;
                }
                if (key == "size") {
                    size_ = v;
                }
            } else if (value.is_string()) {
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()) {
                stats_bool_[key] = value.get<bool>();
            } else if (key == "attributes" && value.is_array()) {
                attributes_.clear();
                for (const auto& attr : value) {
                    if (attr.is_string()) {
                        attributes_.push_back(attr.get<std::string>());
                    }
                }
            }
        }
    };

    if (root.is_array()) {
        for (const auto& monster : root) {
            std::string name = monster.value("name", "");
            if (name == name_ || name.find(name_) == 0) { 
                if (monster.value("hitpoints", 0) > 0) {
                    loadStats(monster);
                    return;
                }
            }
        }
    } else {
        for (auto& [id, monster] : root.items()) {
            if (monster.value("name", "") == name_) {
                loadStats(monster);
                return;
            }
        }
    }
}
