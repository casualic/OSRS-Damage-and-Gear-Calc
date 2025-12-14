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
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        int price_ {0};
        
        // Helper to parse stats from a JSON object
        void parseItemJSON(const json& item);

    public:
        Item() = default;
        Item(std::string n);
        Item(int id);
        
        void fetchStats(const std::string &filepath);
        void fetchStats(int id, const json& allItems);
        void loadFromJSON(const json& itemData);
        
        // Setters for WASM
        void setInt(const std::string& key, int value) { stats_int_[key] = value; }
        void setStr(const std::string& key, const std::string& value) { stats_str_[key] = value; }
        void setBool(const std::string& key, bool value) { stats_bool_[key] = value; }
        void setName(const std::string& n) { name_ = n; }
        void setID(int id) { id_ = id; }
        void setPrice(int price) { price_ = price; }
        
        // Getters
        int getPrice() const { return price_; }
        int getID() const { return id_; }
        std::string getName() const { return name_; }

        int getInt(const std::string& key) const { 
            auto it = stats_int_.find(key);
            return (it != stats_int_.end()) ? it->second : 0;
        }
        bool getBool(const std::string& key) const { 
            auto it = stats_bool_.find(key);
            return (it != stats_bool_.end()) ? it->second : false;
        }
        std::string getStr(const std::string& key) const { 
            auto it = stats_str_.find(key);
            return (it != stats_str_.end()) ? it->second : "";
        }
        
#ifndef __EMSCRIPTEN__
        int fetchPrice();
#endif
};
