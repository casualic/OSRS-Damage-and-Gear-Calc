// player.h
#pragma once
#include <string>
#include <map>
#include "item.h"

class Player {
private:
    std::string username;
    std::map<std::string, int> stats_;
    std::map<std::string, Item> gear_; // Stores equipped items by slot (e.g., "head", "body")

public:
    Player(std::string n);
    std::string fetchStats();
    void parseStats(std::string csv_str);
    int getStat(const std::string& skill){return stats_[skill];}
    
    // WikiSync & Gear
    void fetchGearFromClient(); // Connects to WikiSync and saves wikisync_data.json
    void loadGearStats(const std::string& itemDbPath); // Loads IDs from json and fetches stats
    void loadGearStats(const json& itemDb); // Overload for pre-loaded DB
    void equip(const std::string& slot, const Item& item);
    void unequip(const std::string& slot);
    
    // Combat
    int getEffectiveStat(const std::string& stat); // Base stat + gear bonuses
    int getEquipmentBonus(const std::string& bonus); // Sum of gear bonuses
    const std::map<std::string, Item>& getGear() const { return gear_; }
};


std::map<std::string, int> parseCSV(std::string csv_str);
