// player.h
#pragma once
#include <string>
#include <map>

class Player {
private:
    std::string username;
    std::map<std::string, int> stats_;
public:
    Player(std::string n);
    std::string fetchStats();
    void parseStats(std::string csv_str);
    int getStat(const std::string& skill){return stats_[skill];}
    void fetchGear();

};


std::map<std::string, int> parseCSV(std::string csv_str);
