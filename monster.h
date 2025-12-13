#pragma once
#include <iostream>
#include <map>
#include <variant>
#include <vector>

using MonsterValue = std::variant<std::string, int, double, bool>;

class Monster {
    private:
        std::string name_;
        std::map<std::string, int> stats_int_;
        std::map<std::string, std::string> stats_str_;
        std::map<std::string, bool> stats_bool_;
        std::vector<std::string> attributes_;
        int current_hp_ {0};
        int size_ {1}; // Default to 1
    public:
        Monster(std::string n = "");
        void loadFromJSON(const std::string &filepath);
        void fetchStats();
        void parseStats(std::string csv_str);

        // Setters for WASM
        void setInt(const std::string& key, int value);
        void setStr(const std::string& key, const std::string& value) { stats_str_[key] = value; }
        void setBool(const std::string& key, bool value) { stats_bool_[key] = value; }
        void setName(const std::string& n) { name_ = n; }
        void addAttribute(const std::string& attr) { attributes_.push_back(attr); }
        void setSize(int s) { size_ = s; }
        
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
        int getSize() const { return size_; }
        
        // Attribute helpers
        bool isDemon() const { return hasAttribute("demon"); }
        bool isDragon() const { return hasAttribute("dragon"); }
        bool isKalphite() const { return hasAttribute("kalphite"); }
        bool isLeafy() const { return hasAttribute("leafy"); }
        bool isVampyre() const { return hasAttribute("vampyre") || hasAttribute("vampyre1") || hasAttribute("vampyre2") || hasAttribute("vampyre3"); }
        bool isShade() const { return hasAttribute("shade"); }
        bool isXerician() const { return hasAttribute("xerician"); }
        bool isUndead() const { return hasAttribute("undead"); }
};
