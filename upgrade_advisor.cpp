#include "upgrade_advisor.h"
#include "battle.h"
#include <iostream>
#include <algorithm>
#include <cmath>

UpgradeAdvisor::UpgradeAdvisor(Player& p, Monster& m, const json& items, const json& prices)
    : player_(p), monster_(m), itemDb_(items), priceDb_(prices) {
    
    // Manual Price Proxies for Untradeables
    // Item ID -> Tradeable Component ID (for price)
    priceProxies_[22322] = 22477; // Avernic defender -> Avernic defender hilt
    
    // Fixed Price Proxies (e.g. 1gp for untradeables to force suggestion)
    // Item ID -> Price
    fixedPriceProxies_[12018] = 1; // Salve amulet(ei)
}

bool UpgradeAdvisor::isPotentialUpgrade(const Item& candidate, const Item& current) {
    // Check key offensive stats
    std::vector<std::string> offensiveStats = {
        "strength_bonus", "melee_strength", 
        "attack_stab", "attack_slash", "attack_crush",
        "ranged_strength", "magic_damage"
    };

    // If current item is just a placeholder (ID -1 or 0 stats), almost anything is an upgrade
    // But we want to be stricter to avoid suggesting junk
    
    // Heuristic: Candidate must have at least one offensive stat > current
    // AND that stat should be relevant (e.g. > 0)
    
    // Exception: Items with Special Effects that don't show in stats
    std::string name = candidate.getName();
    if (name.find("Dragon hunter lance") != std::string::npos ||
        name.find("Osmumten's fang") != std::string::npos || 
        name.find("Scythe of vitur") != std::string::npos ||
        name.find("Twisted bow") != std::string::npos || 
        name.find("Dragon hunter crossbow") != std::string::npos ||
        name.find("Salve amulet") != std::string::npos) {
        return true;
    }

    for (const auto& stat : offensiveStats) {
        // We use const_cast because getInt is not const in Item (should be fixed later)
        int candVal = const_cast<Item&>(candidate).getInt(stat);
        int currVal = const_cast<Item&>(current).getInt(stat);
        
        if (candVal > currVal) return true;
    }
    
    return false;
}

std::vector<UpgradeSuggestion> UpgradeAdvisor::suggestUpgrades() {
    std::vector<UpgradeSuggestion> suggestions;
    
    // 1. Baseline DPS
    Battle baselineBattle(player_, monster_);
    double currentDps = baselineBattle.solveOptimalDPS();
    
    std::cout << "Calculating upgrades... (Current DPS: " << currentDps << ")\n";

    // 2. Iterate all items
    int processed = 0;
    int total = itemDb_.size();

    for (auto& [idStr, itemData] : itemDb_.items()) {
        processed++;
        if (processed % 1000 == 0) std::cout << "\rScanning items: " << processed << "/" << total << std::flush;

        // Basic filters
        bool isTradeable = itemData.value("tradeable_on_ge", false);
        int id = std::stoi(idStr);
        
        // Special check for price proxies (allow if in proxy list)
        bool hasProxy = (priceProxies_.find(id) != priceProxies_.end());
        bool hasFixedProxy = (fixedPriceProxies_.find(id) != fixedPriceProxies_.end());
        
        if (!isTradeable && !hasProxy && !hasFixedProxy) continue;
        if (!itemData.value("equipable_by_player", false)) continue;
        
        // Create Item object
        Item candidate(id);
        candidate.fetchStats(id, itemDb_);
        
        // Check slot
        std::string rawSlot = candidate.getStr("slot");
        if (rawSlot.empty()) continue; 
        
        // Logic for 2H weapons:
        // - rawSlot "2h" maps to gear slot "weapon"
        // - rawSlot "weapon" maps to gear slot "weapon"
        // - rawSlot "shield" maps to gear slot "shield"
        
        std::string targetSlot = rawSlot;
        if (targetSlot == "2h") targetSlot = "weapon";
        
        // Get current item in this slot
        Item currentItem("Empty");
        auto currentGear = player_.getGear();
        if (currentGear.count(targetSlot)) {
            currentItem = currentGear.at(targetSlot);
        }
        
        // Skip if same item
        if (candidate.getID() == currentItem.getID()) continue;

        // Optimization: Pre-filter based on stats before running simulation
        if (!isPotentialUpgrade(candidate, currentItem)) continue;

        // Get Price
        int price = 0;
        
        // 1. Check Fixed Price Proxy
        if (fixedPriceProxies_.count(id)) {
            price = fixedPriceProxies_.at(id);
        } else {
            // 2. Check Item/Component Price
            int priceId = id;
            if (priceProxies_.count(id)) {
                priceId = priceProxies_.at(id);
            }
            
            std::string idKey = std::to_string(priceId);
            if (priceDb_["data"].contains(idKey)) {
                 int high = priceDb_["data"][idKey].value("high", 0);
                 int low = priceDb_["data"][idKey].value("low", 0);
                 if (high > 0 && low > 0) price = (high + low) / 2;
                 else if (high > 0) price = high;
                 else price = low;
            }
        }
        
        // Filter out absurdly cheap items (junk) or unpriced items if we want strictness
        // Allowing cheap items for now, but price must be > 0 to calc efficiency
        if (price <= 0) continue;

        // 3. Simulate
        // Clone player
        Player pClone = player_; 
        
        // Handle 2H logic
        if (rawSlot == "2h") {
            // If equipping 2H, must remove shield
            pClone.unequip("shield");
            pClone.equip("weapon", candidate);
        } else if (targetSlot == "shield") {
            // If equipping shield, check if we have a 2H weapon
            // If so, we must unequip the weapon (simulate 1h punching + shield? or ignore?)
            // Usually we wouldn't suggest a shield if we are main-handing a 2h unless we also suggest a 1h.
            // But for atomic upgrades, we'll assume we unequip the 2H weapon (fallback to unarmed + shield).
            // This will likely result in a DPS loss, so it won't be suggested. Safe.
            if (pClone.getGear().count("weapon")) {
                Item w = pClone.getGear().at("weapon");
                if (w.getStr("slot") == "2h") {
                    pClone.unequip("weapon");
                }
            }
            pClone.equip("shield", candidate);
        } else {
            // Standard slot (including 1h weapon)
            pClone.equip(targetSlot, candidate);
        }
        
        Battle simBattle(pClone, monster_);
        double newDps = simBattle.solveOptimalDPS();
        
        if (newDps > currentDps) {
            double increase = newDps - currentDps;
            double efficiency = (increase / price) * 1000000.0; // DPS increase per 1M GP
            
            suggestions.push_back({
                candidate.getName(),
                id,
                rawSlot,
                price,
                currentDps,
                newDps,
                increase,
                efficiency
            });
        }
    }
    std::cout << "\rScanning items: Done!              \n";

    // Sort
    std::sort(suggestions.begin(), suggestions.end()); // Uses < operator defined in struct

    return suggestions;
}

