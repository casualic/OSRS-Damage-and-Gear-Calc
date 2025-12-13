#include <iostream>
#include <fstream>
#include <iomanip>
#include "player.h"
#include "monster.h"
#include "battle.h"
#include "upgrade_advisor.h"
#include "json.hpp"

using json = nlohmann::json;

json loadJSON(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Warning: Could not open " << filename << "\n";
        return json::object();
    }
    try {
        return json::parse(f);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing " << filename << ": " << e.what() << "\n";
        return json::object();
    }
}

int main() {
    try {
        std::cout << "=== OSRS DPS Calculator ===\n";

        // 0. Load Databases
        std::cout << "[0/6] Loading Databases...\n";
        std::cout << "      Loading items-complete.json... ";
        json itemDb = loadJSON("items-complete.json");
        std::cout << "Done (" << itemDb.size() << " items)\n";
        
        std::cout << "      Loading latest_prices.json... ";
        json priceDb = loadJSON("latest_prices.json");
        // prices are usually in "data" key
        std::cout << "Done\n";

        // 1. Setup Player
        std::cout << "[1/6] Initializing Player 'WolpiXD'...\n";
        Player player("WolpiXD"); 
        
        // 2. Fetch Skills
        std::cout << "[2/6] Fetching Skills from HiScores...\n";
        std::string stats = player.fetchStats();
        if (stats.empty()) {
            std::cerr << "Failed to fetch stats. Using mock stats for testing.\n";
            // Mock stats: 99s combat
            // Format: Rank,Level,XP (repeated for 24 skills)
            // Just repeating "1,99,1" for all skills
            std::string mockStatLine = "1,99,1\n";
            for(int i=0; i<24; ++i) stats += mockStatLine;
        }
        player.parseStats(stats);
        std::cout << "      Attack: " << player.getStat("Attack") << " | Strength: " << player.getStat("Strength") << "\n";

        // 3. Fetch Gear
        std::cout << "[3/6] Fetching Gear from WikiSync...\n";
        player.fetchGearFromClient();
        
        std::cout << "      Loading Gear Stats from DB...\n";
        player.loadGearStats(itemDb);
        
        // 4. Setup Monster
        std::string monsterName = "Vorkath";
        std::cout << "[4/6] Initializing Monster '" << monsterName << "'...\n";
        Monster monster(monsterName); 
        
        // Try loading from standard DB
        monster.loadFromJSON("monsters-nodrops.json");
        
        // If not found (HP is 0), try bosses DB
        if (monster.getCurrentHP() == 0) {
             std::cout << "      Not found in standard DB, checking bosses DB...\n";
             monster.loadFromJSON("bosses_complete.json");
        }
        
        // Verification check
        if (monster.getCurrentHP() > 0) {
             std::cout << "      Found " << monsterName << " with " << monster.getCurrentHP() << " HP.\n";
        } else {
            std::cerr << "      Warning: '" << monsterName << "' not found. Defaulting to 'Goblin'.\n";
            monster = Monster("Goblin");
            monster.loadFromJSON("monsters-nodrops.json");
        }

        // 5. Battle
        std::cout << "[5/6] Starting Battle Simulation...\n";
        Battle battle(player, monster);
        battle.optimizeAttackStyle();
        battle.runSimulations(10000);

        // 6. Upgrade Advisor
        std::cout << "\n[6/6] Generating Next Best Item Suggestions...\n";
        if (priceDb.empty() || itemDb.empty()) {
             std::cerr << "Cannot run Advisor: Missing Item or Price DB.\n";
        } else {
            UpgradeAdvisor advisor(player, monster, itemDb, priceDb);
            auto suggestions = advisor.suggestUpgrades();

            // Split into Singles and Duos
            std::vector<UpgradeSuggestion> singles;
            std::vector<UpgradeSuggestion> duos;
            for(const auto& s : suggestions) {
                if(s.itemNames.size() > 1) duos.push_back(s);
                else singles.push_back(s);
            }

            // --- Singles ---
            std::cout << "\n=== Top 10 SINGLE Upgrades (Efficiency) ===\n";
            std::cout << std::left << std::setw(50) << "Item Name" 
                      << " | " << std::setw(20) << "Slot"
                      << " | " << std::setw(10) << "Price"
                      << " | " << std::setw(10) << "+DPS"
                      << " | " << "DPS/1M GP" << "\n";
            std::cout << std::string(115, '-') << "\n";
            
            std::sort(singles.begin(), singles.end()); // Efficient sort
            int count = 0;
            for (const auto& sug : singles) {
                if (count++ >= 10) break;
                std::cout << std::left << std::setw(50) << sug.itemNames[0].substr(0, 49)
                          << " | " << std::setw(20) << sug.slots[0]
                          << " | " << std::setw(10) << sug.price
                          << " | " << std::setw(10) << std::fixed << std::setprecision(3) << sug.dpsIncrease
                          << " | " << std::fixed << std::setprecision(3) << sug.dpsPerMillionGP << "\n";
            }

            // --- Duos ---
            std::cout << "\n=== Top 10 DUO Upgrades (Efficiency) ===\n";
            std::cout << std::left << std::setw(50) << "Item Name" 
                      << " | " << std::setw(20) << "Slot"
                      << " | " << std::setw(10) << "Price"
                      << " | " << std::setw(10) << "+DPS"
                      << " | " << "DPS/1M GP" << "\n";
            std::cout << std::string(115, '-') << "\n";
            
            std::sort(duos.begin(), duos.end()); // Efficient sort
            count = 0;
            for (const auto& sug : duos) {
                if (count++ >= 10) break;
                std::string nameStr = sug.itemNames[0] + " + " + sug.itemNames[1];
                std::string slotStr = sug.slots[0] + "+" + sug.slots[1];

                std::cout << std::left << std::setw(50) << nameStr.substr(0, 49)
                          << " | " << std::setw(20) << slotStr
                          << " | " << std::setw(10) << sug.price
                          << " | " << std::setw(10) << std::fixed << std::setprecision(3) << sug.dpsIncrease
                          << " | " << std::fixed << std::setprecision(3) << sug.dpsPerMillionGP << "\n";
            }
            
            // --- Highest DPS Singles ---
            std::sort(singles.begin(), singles.end(), [](const UpgradeSuggestion& a, const UpgradeSuggestion& b) {
                return a.dpsIncrease > b.dpsIncrease;
            });
            std::cout << "\n=== Top 10 SINGLE Upgrades (Max DPS) ===\n";
            // ... (print loop similar to above)
            count = 0;
            for (const auto& sug : singles) {
                if (count++ >= 10) break;
                std::cout << std::left << std::setw(50) << sug.itemNames[0].substr(0, 49)
                          << " | " << std::setw(20) << sug.slots[0]
                          << " | " << std::setw(10) << sug.price
                          << " | " << std::setw(10) << std::fixed << std::setprecision(3) << sug.dpsIncrease
                          << " | " << std::fixed << std::setprecision(3) << sug.dpsPerMillionGP << "\n";
            }


            // --- Highest DPS Duos ---
             std::sort(duos.begin(), duos.end(), [](const UpgradeSuggestion& a, const UpgradeSuggestion& b) {
                return a.dpsIncrease > b.dpsIncrease;
            });
            std::cout << "\n=== Top 10 DUO Upgrades (Max DPS) ===\n";
             count = 0;
            for (const auto& sug : duos) {
                if (count++ >= 10) break;
                std::string nameStr = sug.itemNames[0] + " + " + sug.itemNames[1];
                std::string slotStr = sug.slots[0] + "+" + sug.slots[1];
                std::cout << std::left << std::setw(50) << nameStr.substr(0, 49)
                          << " | " << std::setw(20) << slotStr
                          << " | " << std::setw(10) << sug.price
                          << " | " << std::setw(10) << std::fixed << std::setprecision(3) << sug.dpsIncrease
                          << " | " << std::fixed << std::setprecision(3) << sug.dpsPerMillionGP << "\n";
            }

            if (suggestions.empty()) {
                std::cout << "No upgrades found (or Price DB missing/outdated).\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "\nFatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
