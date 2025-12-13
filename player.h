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
    
    // State flags
    bool onSlayerTask_ {false};
    bool piety_ {false};
    bool rigour_ {false};
    bool superCombat_ {false};
    int currentHP_ {99};
    int maxHP_ {99};

public:
    Player(std::string n = "");
    void parseStats(std::string csv_str);
    int getStat(const std::string& skill) { return stats_[skill]; }
    void setStat(const std::string& skill, int level) { stats_[skill] = level; }
    
    // Prayers & Potions
    void setPiety(bool active) { piety_ = active; }
    bool isPietyActive() const { return piety_; }
    void setRigour(bool active) { rigour_ = active; }
    bool isRigourActive() const { return rigour_; }
    void setSuperCombat(bool active) { superCombat_ = active; }
    bool isSuperCombatActive() const { return superCombat_; }
    
    // Helper to get level including potion boosts
    int getBoostedLevel(const std::string& skill);

    std::string getUsername() const { return username; }
    void setUsername(const std::string& name) { username = name; }
    
    // Gear management
    void equip(const std::string& slot, const Item& item);
    void unequip(const std::string& slot);
    void clearGear() { gear_.clear(); }
    bool hasItem(const std::string& slot) const { return gear_.count(slot) > 0; }
    Item getEquippedItem(const std::string& slot) const;
    bool hasEquipped(const std::string& itemName) const;
    
    // Combat
    int getEffectiveStat(const std::string& stat); // Base stat + gear bonuses
    int getEquipmentBonus(const std::string& bonus); // Sum of gear bonuses
    const std::map<std::string, Item>& getGear() const { return gear_; }
    
    // State Management
    void setSlayerTask(bool onTask) { onSlayerTask_ = onTask; }
    bool isOnSlayerTask() const { return onSlayerTask_; }
    
    void setHP(int current, int max) { currentHP_ = current; maxHP_ = max; }
    int getCurrentHP() const { return currentHP_; }
    int getMaxHP() const { return maxHP_; }
    
    // Set Bonus Helper
    std::string getActiveSet();
    int countCrystalPieces();
    
#ifndef __EMSCRIPTEN__
    // Network methods - only available in native builds
    std::string fetchStats();
    void fetchGearFromClient();
    void loadGearStats(const std::string& itemDbPath);
    void loadGearStats(const json& itemDb);
#endif
};

std::map<std::string, int> parseCSV(std::string csv_str);
