#include <iostream>
#include "player.h"
#include "monster.h"
#include "battle.h"

int main() {
    try {
        std::cout << "=== OSRS DPS Calculator ===\n";

        // 1. Setup Player
        std::cout << "[1/5] Initializing Player 'WolpiXD'...\n";
        Player player("WolpiXD"); 
        
        // 2. Fetch Skills
        std::cout << "[2/5] Fetching Skills from HiScores...\n";
        std::string stats = player.fetchStats();
        if (stats.empty()) {
            std::cerr << "Failed to fetch stats. Check internet or username.\n";
            return 1;
        }
        player.parseStats(stats);
        std::cout << "      Attack: " << player.getStat("Attack") << " | Strength: " << player.getStat("Strength") << "\n";

        // 3. Fetch Gear
        std::cout << "[3/5] Fetching Gear from WikiSync...\n";
        player.fetchGearFromClient();
        
        std::cout << "      Loading Gear Stats from DB...\n";
        player.loadGearStats("items-complete.json");
        
        // 4. Setup Monster
        std::string monsterName = "Vorkath";
        std::cout << "[4/5] Initializing Monster '" << monsterName << "'...\n";
        Monster monster(monsterName); 
        monster.loadFromJSON("monsters-nodrops.json");
        
        // Verification check
        try {
             int hp = monster.getInt("hitpoints"); // Will throw if not loaded
             std::cout << "      Found " << monsterName << " with " << hp << " HP.\n";
        } catch (...) {
            std::cerr << "      Warning: '" << monsterName << "' not found. Defaulting to 'Goblin'.\n";
            monster = Monster("Goblin");
            monster.loadFromJSON("monsters-nodrops.json");
        }

        // 5. Battle
        std::cout << "[5/5] Starting Battle Simulation...\n";
        Battle battle(player, monster);
        battle.runSimulations(10000);

    } catch (const std::exception& e) {
        std::cerr << "\nFatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
