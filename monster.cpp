#include "monster.h"
#include <iostream>
#include "external/simdjson.h"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;


Monster::Monster(std::string n) : name_(std::move(n)) {};

void Monster::loadFromJSON(const std::string &filepath){
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

    // Helper to load stats from a found JSON object
    auto loadStats = [&](const json& monster) {
        // Clear previous stats if overwriting? Or merge?
        // For now, simple overwrite/add
        for (auto& [key, value] : monster.items()){
            if (value.is_number_integer()){
                int v = value.get<int>();
                stats_int_[key] = v;
                if (key == "hitpoints") {
                    current_hp_ = v; // Set, don't add (fix logic)
                }
            } else if (value.is_string()){
                stats_str_[key] = value.get<std::string>();
            } else if (value.is_boolean()){
                stats_bool_[key] = value.get<bool>();
            } else if (key == "attributes" && value.is_array()) {
                // Parse attributes array
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
        // Handle array format (bosses_complete.json)
        for (const auto& monster : root) {
            std::string name = monster.value("name", "");
            // Exact match or Prefix match (e.g. "Vorkath" matches "Vorkath (Post-quest)")
            if (name == name_ || name.find(name_) == 0) { 
                // Prioritize "Post-quest" versions if multiple match?
                // The loop order matters. The file seems sorted alphabetically.
                // "Vorkath (Dragon Slayer II)" comes before "Vorkath (Post-quest)".
                // We probably want Post-quest usually.
                // For now, take the first valid match that has non-zero HP?
                
                if (monster.value("hitpoints", 0) > 0) {
                     loadStats(monster);
                     std::cout << "Loaded stats for '" << name << "' from " << filepath << "\n";
                     return; // Stop after first good match
                }
            }
        }
    } else {
        // Handle Map format (monsters-nodrops.json)
        for (auto& [id, monster] : root.items()){
            if (monster.value("name", "") == name_){
                loadStats(monster);
                std::cout << "Loaded stats for '" << name_ << "' from " << filepath << "\n";
                return;
            }
        }
    }
}

bool Monster::hasAttribute(const std::string& attr) const {
    for (const auto& a : attributes_) {
        // Case insensitive comparison? For now assume exact or standard lowercase
        if (a == attr) return true;
    }
    return false;
}
