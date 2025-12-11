#pragma once
#include <iostream>
#include <string>
#include <map>

class Item {
    private:
        std::string name_;
        std::map<std::string, int> stats_;
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        int price_;
    public:
        Item(std::string n);
        int fetchPrice();
        void fetchStats(const std::string &filepath);
        int getPrice() const { return price_; }
        int getInt(std::string key) { return stats_int_[key];}
        bool getBool(std::string key) { return stats_bool_[key];}
        std::string getStr(std::string key) { return stats_str_[key];}
        void fetchGear(); // fetches gear from player via the RuneLite websocket connection
        
};
