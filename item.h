#pragma once
#include <iostream>
#include <string>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

class Item {
    private:
        int id_ {-1};
        std::string name_;
        std::map<std::string, int> stats_;
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        int price_;
        
        // Helper to parse stats from a JSON object
        void parseItemJSON(const json& item);

    public:
        Item() = default;
        Item(std::string n);
        Item(int id);
        
        int fetchPrice();
        void fetchStats(const std::string &filepath);
        void fetchStats(int id, const json& allItems); // New overload
        
        int getPrice() const { return price_; }
        int getID() const { return id_; }
        std::string getName() const { return name_; }

        int getInt(std::string key) { 
            if (stats_int_.find(key) != stats_int_.end()) return stats_int_[key];
            return 0;
        }
        bool getBool(std::string key) { 
            if (stats_bool_.find(key) != stats_bool_.end()) return stats_bool_[key];
            return false;
        }
        std::string getStr(std::string key) { 
            if (stats_str_.find(key) != stats_str_.end()) return stats_str_[key];
            return "";
        }
        
        // Removed fetchGear() as it's now in Player
};
