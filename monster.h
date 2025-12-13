#pragma once
#include <iostream>
#include <map>
#include <variant>
#include <vector>

using MonsterValue = std::variant<std::string, int, double, bool>;

class Monster {
    private:
        std::string name_;
        // std::map<std::string, MonsterValue> stats_;
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        std::vector<std::string> attributes_;
        int current_hp_ {0};
    public:
        Monster(std::string n);
        void loadFromJSON(const std::string &filepath);
        void fetchStats();
        void parseStats(std::string csv_str);
        //Stats calling methods
        int getInt(const std::string& key) const {
            auto it = stats_int_.find(key);
            if (it != stats_int_.end()) return it->second;
            return 0;
        }
        std::string getStr(const std::string& key) const {
            auto it = stats_str_.find(key);
            if (it != stats_str_.end()) return it->second;
            return "";
        }
        bool getBool(const std::string& key) const {
            auto it = stats_bool_.find(key);
            if (it != stats_bool_.end()) return it->second;
            return false;
        }
        bool hasAttribute(const std::string& attr) const;
        int getCurrentHP() const { return current_hp_; }
        void takeDMG(int a) { current_hp_ -= a; }


    };