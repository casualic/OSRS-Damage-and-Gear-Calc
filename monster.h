#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <variant>

using MonsterValue = std::variant<std::string, int, double, bool>;

class Monster {
    private:
        std::string name_;
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        std::vector<std::string> attributes_;
        int current_hp_ {0};
    public:
        Monster(std::string n = "");
        void loadFromJSON(const std::string &filepath);
        
        // Setters for WASM
        void setInt(const std::string& key, int value);
        void setStr(const std::string& key, const std::string& value) { stats_str_[key] = value; }
        void setBool(const std::string& key, bool value) { stats_bool_[key] = value; }
        void setName(const std::string& n) { name_ = n; }
        void addAttribute(const std::string& attr) { attributes_.push_back(attr); }
        
        // Getters
        int getInt(const std::string& key) const;
        std::string getStr(const std::string& key) const;
        bool getBool(const std::string& key) const;
        bool hasInt(const std::string& key) const { return stats_int_.count(key) > 0; }
        bool hasAttribute(const std::string& attr) const;
        std::string getName() const { return name_; }
        int getCurrentHP() const { return current_hp_; }
        void setCurrentHP(int hp) { current_hp_ = hp; }
        void takeDMG(int a) { current_hp_ -= a; }
        void resetHP();
};