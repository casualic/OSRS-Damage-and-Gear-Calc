// wasm_bindings.cpp
// Emscripten bindings for OSRS DPS Calculator

#ifdef __EMSCRIPTEN__

#include <emscripten/bind.h>
#include "player.h"
#include "monster.h"
#include "item.h"
#include "battle.h"
#include "upgrade_advisor.h"

using namespace emscripten;

// Wrapper for UpgradeAdvisor that accepts JSON strings
class UpgradeAdvisorWrapper {
private:
    Player player_;
    Monster monster_;
    json itemDb_;
    json priceDb_;
    
public:
    void initialize(const Player& player, const Monster& monster, 
                    const std::string& itemDbJson, const std::string& priceDbJson) {
        try {
            player_ = player;
            monster_ = monster;
            itemDb_ = json::parse(itemDbJson);
            priceDb_ = json::parse(priceDbJson);
        } catch (const std::exception& e) {
            std::cerr << "Error initializing UpgradeAdvisor: " << e.what() << "\n";
        }
    }
    
    std::string suggestUpgrades(int maxPrice) {
        try {
            UpgradeAdvisor advisor(player_, monster_, itemDb_, priceDb_);
            auto suggestions = advisor.suggestUpgrades();
            
            json result = json::array();
            for (const auto& sug : suggestions) {
                // Filter by max price if specified
                if (maxPrice > 0 && sug.price > maxPrice) continue;
                
                // Handle duo suggestions (arrays)
                json itemNamesArr = json::array();
                json itemIdsArr = json::array();
                json slotsArr = json::array();
                
                for (const auto& name : sug.itemNames) {
                    itemNamesArr.push_back(name);
                }
                for (const auto& id : sug.itemIds) {
                    itemIdsArr.push_back(id);
                }
                for (const auto& slot : sug.slots) {
                    slotsArr.push_back(slot);
                }
                
                result.push_back({
                    {"itemNames", itemNamesArr},
                    {"itemIds", itemIdsArr},
                    {"slots", slotsArr},
                    {"price", sug.price},
                    {"oldDps", sug.oldDps},
                    {"newDps", sug.newDps},
                    {"dpsIncrease", sug.dpsIncrease},
                    {"dpsPerMillionGP", sug.dpsPerMillionGP},
                    {"isDuo", sug.itemNames.size() > 1}
                });
            }
            
            return result.dump();
        } catch (const std::exception& e) {
            std::cerr << "Error in suggestUpgrades: " << e.what() << "\n";
            return "[]";
        }
    }
    
    double getBaseDPS() {
        Battle battle(player_, monster_);
        return battle.solveOptimalDPS();
    }
};

// Helper to load item from JSON string
void loadItemFromJson(Item& item, int id, const std::string& allItemsJson) {
    try {
        json allItems = json::parse(allItemsJson);
        item.fetchStats(id, allItems);
    } catch (const std::exception& e) {
        std::cerr << "Error loading item: " << e.what() << "\n";
    }
}

// Helper to load monster from JSON string (for bosses array format)
void loadMonsterFromJson(Monster& monster, const std::string& name, const std::string& jsonData) {
    try {
        json root = json::parse(jsonData);
        
        auto loadStats = [&monster](const json& m) {
            for (auto& [key, value] : m.items()) {
                if (value.is_number_integer()) {
                    monster.setInt(key, value.get<int>());
                } else if (value.is_string()) {
                    monster.setStr(key, value.get<std::string>());
                } else if (value.is_boolean()) {
                    monster.setBool(key, value.get<bool>());
                } else if (key == "attributes" && value.is_array()) {
                    for (const auto& attr : value) {
                        if (attr.is_string()) {
                            monster.addAttribute(attr.get<std::string>());
                        }
                    }
                } else if (key == "size" && value.is_number_integer()) {
                    monster.setSize(value.get<int>());
                }
            }
        };
        
        monster.setName(name);
        
        if (root.is_array()) {
            for (const auto& m : root) {
                std::string mName = m.value("name", "");
                if (mName == name || mName.find(name) == 0) {
                    if (m.value("hitpoints", 0) > 0) {
                        loadStats(m);
                        return;
                    }
                }
            }
        } else {
            for (auto& [id, m] : root.items()) {
                if (m.value("name", "") == name) {
                    loadStats(m);
                    return;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading monster: " << e.what() << "\n";
    }
}

// Get battle results as JSON string
std::string getBattleResultsJson(Battle& battle) {
    BattleResult result = battle.getResults();
    
    json j = {
        {"dps", result.dps},
        {"maxHit", result.maxHit},
        {"hitChance", result.hitChance},
        {"style", result.style},
        {"stance", result.stance},
        {"attackSpeed", result.attackSpeed},
        {"avgTTK", result.avgTTK},
        {"killsPerHour", result.killsPerHour},
        {"isFang", result.isFang},
        {"isDHL", result.isDHL},
        {"isDHCB", result.isDHCB},
        {"isArclight", result.isArclight},
        {"isKeris", result.isKeris},
        {"isScythe", result.isScythe},
        {"isTbow", result.isTbow},
        {"hasSalve", result.hasSalve},
        {"onTask", result.onTask},
        {"activeSet", result.activeSet}
    };
    
    return j.dump();
}

// Helper factory functions for Item constructors
Item createItemFromString(const std::string& name) {
    return Item(name);
}

Item createItemFromInt(int id) {
    return Item(id);
}

EMSCRIPTEN_BINDINGS(osrs_calc) {
    // Item class
    class_<Item>("Item")
        .constructor<>()
        .constructor<int>()
        .function("getID", &Item::getID)
        .function("getName", &Item::getName)
        .function("getPrice", &Item::getPrice)
        .function("getInt", &Item::getInt)
        .function("getBool", &Item::getBool)
        .function("getStr", &Item::getStr)
        .function("setInt", &Item::setInt)
        .function("setStr", &Item::setStr)
        .function("setBool", &Item::setBool)
        .function("setName", &Item::setName)
        .function("setID", &Item::setID)
        .function("setPrice", &Item::setPrice);
    
    // Player class
    class_<Player>("Player")
        .constructor<>()
        .constructor<std::string>()
        .function("getUsername", &Player::getUsername)
        .function("setUsername", &Player::setUsername)
        .function("getStat", &Player::getStat)
        .function("setStat", &Player::setStat)
        .function("parseStats", &Player::parseStats)
        .function("equip", &Player::equip)
        .function("unequip", &Player::unequip)
        .function("clearGear", &Player::clearGear)
        .function("hasItem", &Player::hasItem)
        .function("getEquippedItem", &Player::getEquippedItem)
        .function("getEffectiveStat", &Player::getEffectiveStat)
        .function("getEquipmentBonus", &Player::getEquipmentBonus)
        .function("setSlayerTask", &Player::setSlayerTask)
        .function("isOnSlayerTask", &Player::isOnSlayerTask)
        .function("setHP", &Player::setHP)
        .function("getCurrentHP", &Player::getCurrentHP)
        .function("getMaxHP", &Player::getMaxHP)
        .function("getActiveSet", &Player::getActiveSet)
        .function("setPiety", &Player::setPiety)
        .function("isPietyActive", &Player::isPietyActive)
        .function("setRigour", &Player::setRigour)
        .function("isRigourActive", &Player::isRigourActive)
        .function("setSuperCombat", &Player::setSuperCombat)
        .function("isSuperCombatActive", &Player::isSuperCombatActive);
    
    // Monster class
    class_<Monster>("Monster")
        .constructor<>()
        .constructor<std::string>()
        .function("getName", &Monster::getName)
        .function("setName", &Monster::setName)
        .function("getInt", &Monster::getInt)
        .function("getStr", &Monster::getStr)
        .function("getBool", &Monster::getBool)
        .function("setInt", &Monster::setInt)
        .function("setStr", &Monster::setStr)
        .function("setBool", &Monster::setBool)
        .function("addAttribute", &Monster::addAttribute)
        .function("hasAttribute", &Monster::hasAttribute)
        .function("getCurrentHP", &Monster::getCurrentHP)
        .function("setCurrentHP", &Monster::setCurrentHP)
        .function("resetHP", &Monster::resetHP)
        .function("hasInt", &Monster::hasInt)
        .function("setSize", &Monster::setSize)
        .function("getSize", &Monster::getSize)
        .function("isDemon", &Monster::isDemon)
        .function("isDragon", &Monster::isDragon)
        .function("isUndead", &Monster::isUndead)
        .function("isKalphite", &Monster::isKalphite)
        .function("isLeafy", &Monster::isLeafy)
        .function("isVampyre", &Monster::isVampyre)
        .function("isXerician", &Monster::isXerician)
        .function("isShade", &Monster::isShade);
    
    // Battle class
    class_<Battle>("Battle")
        .constructor<Player&, Monster&>()
        .function("simulate", &Battle::simulate)
        .function("runSimulations", &Battle::runSimulations)
        .function("optimizeAttackStyle", &Battle::optimizeAttackStyle)
        .function("solveOptimalDPS", &Battle::solveOptimalDPS)
        .function("getStyle", &Battle::getStyle)
        .function("getAttackSpeed", &Battle::getAttackSpeed)
        .function("getMaxHit", &Battle::getMaxHit)
        .function("getHitChance", &Battle::getHitChance)
        .function("getDPS", &Battle::getDPS)
        .function("hasFang", &Battle::hasFang)
        .function("hasDHL", &Battle::hasDHL)
        .function("hasAnySalve", &Battle::hasAnySalve);
    
    // UpgradeAdvisorWrapper class
    class_<UpgradeAdvisorWrapper>("UpgradeAdvisor")
        .constructor<>()
        .function("initialize", &UpgradeAdvisorWrapper::initialize)
        .function("suggestUpgrades", &UpgradeAdvisorWrapper::suggestUpgrades)
        .function("getBaseDPS", &UpgradeAdvisorWrapper::getBaseDPS);
    
    // Helper functions
    function("loadItemFromJson", &loadItemFromJson);
    function("loadMonsterFromJson", &loadMonsterFromJson);
    function("getBattleResultsJson", &getBattleResultsJson);
}

#endif // __EMSCRIPTEN__
