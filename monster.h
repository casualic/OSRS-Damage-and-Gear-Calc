#pragma once
#include <iostream>
#include <map>

using MonsterValue = std::variant<std::string, int, double, bool>;

class Monster {
    private:
    std::string name_;
    std::map<std::string, MonsterValue> stats_;
    public:
    Monster(std::string n);
    void loadFromJSON(const std::string &filepath);
    void fetchStats();
    void parseStats(std::string csv_str);
    void loadFromJSON(const std::string filepath);

    // Template getter for type-safe access
    template<typename T>
    T getStat(const std::string& skill){
        return std::get<T>(stats_[skill]);
    }
};