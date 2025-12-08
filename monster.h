#pragma once
#include <iostream>
#include <map>

class Monster {
    private:
    std::string name_;
    std::map<std::string, int> stats_;
    public:
    Monster(std::string n);
    std::string fetchStats();
    void parseStats(std::string csv_str);
    int getStat(const std::string& skill){return stats_[skill];
    
    }
}