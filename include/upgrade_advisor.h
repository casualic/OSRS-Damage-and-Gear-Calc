#pragma once
#include "player.h"
#include "monster.h"
#include "json.hpp"
#include <vector>
#include <string>

using json = nlohmann::json;

struct UpgradeSuggestion {
    std::vector<std::string> itemNames;
    std::vector<int> itemIds;
    std::vector<std::string> slots;
    int price; // Total price
    double oldDps;
    double newDps;
    double dpsIncrease;
    double dpsPerMillionGP;
    
    // Sort descending by dpsPerMillionGP
    bool operator<(const UpgradeSuggestion& other) const {
        return dpsPerMillionGP > other.dpsPerMillionGP;
    }
};

class UpgradeAdvisor {
private:
    Player& player_;
    Monster& monster_;
    const json& itemDb_;
    const json& priceDb_;
    std::map<int, int> priceProxies_;
    std::map<int, int> fixedPriceProxies_;

    // Helper to check if item is a potential upgrade
    bool isPotentialUpgrade(const Item& candidate, const Item& current);

public:
    UpgradeAdvisor(Player& p, Monster& m, const json& items, const json& prices);
    
    std::vector<UpgradeSuggestion> suggestUpgrades(bool excludeThrowables = false, bool excludeAmmo = false);
};
