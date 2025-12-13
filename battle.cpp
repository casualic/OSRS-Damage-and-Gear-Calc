// battle.cpp
#include "battle.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>

Battle::Battle(Player& p, Monster& m) : player_(p), monster_(m) {
    init();
    determineStyle();
}

Battle::Battle(const Player& p, const Monster& m) : player_(p), monster_(m) {
    init();
    determineStyle();
}

void Battle::init() {
    std::random_device rd;
    gen = std::mt19937(rd());
    
    // Default values
    attack_speed_ = 4;
    isFang_ = false;
    isDHL_ = false;
    isDHCB_ = false;
    isArclight_ = false;
    isKeris_ = false;
    isKerisBreaching_ = false;
    isLeafBladed_ = false;
    isScythe_ = false;
    isTbow_ = false;
    isDharok_ = false;
    
    hasSalve_ = false;
    hasSalveE_ = false;
    hasSalveI_ = false;
    hasSalveEI_ = false;
    
    // Monster Attributes
    isDragon_ = monster_.isDragon();
    isUndead_ = monster_.isUndead();
    isDemon_ = monster_.isDemon();
    isKalphite_ = monster_.isKalphite();
    isLeafy_ = monster_.isLeafy();
    isVampyre_ = monster_.isVampyre();
    isShade_ = monster_.isShade();
    isXerician_ = monster_.isXerician();
    
    // Player State
    isOnTask_ = player_.isOnSlayerTask();
    activeSet_ = player_.getActiveSet();
    
    // Parse Gear
    auto gear = player_.getGear();
    
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed_ = speed;
        
        std::string name = weapon.getName();
        
        if (name.find("Osmumten's fang") != std::string::npos) isFang_ = true;
        if (name.find("Dragon hunter lance") != std::string::npos) isDHL_ = true;
        if (name.find("Dragon hunter crossbow") != std::string::npos) isDHCB_ = true;
        if (name.find("Arclight") != std::string::npos || name.find("Emberlight") != std::string::npos) isArclight_ = true;
        if (name.find("Keris") != std::string::npos) {
            isKeris_ = true;
            if (name.find("breaching") != std::string::npos) isKerisBreaching_ = true;
        }
        if (name.find("Leaf-bladed") != std::string::npos) isLeafBladed_ = true;
        if (name.find("Scythe of vitur") != std::string::npos) isScythe_ = true;
        if (name.find("Twisted bow") != std::string::npos) isTbow_ = true;
        
        if (activeSet_ == "Dharok" && name.find("Dharok's greataxe") != std::string::npos) {
            isDharok_ = true;
        }
    }
    
    if (gear.count("neck")) {
        std::string neck = gear.at("neck").getName();
        if (neck.find("Salve amulet") != std::string::npos) {
            if (neck.find("(ei)") != std::string::npos) hasSalveEI_ = true;
            else if (neck.find("(i)") != std::string::npos) hasSalveI_ = true;
            else if (neck.find("(e)") != std::string::npos) hasSalveE_ = true;
            else hasSalve_ = true;
        }
    }

    stance_bonus_attack_ = 3;
    stance_bonus_strength_ = 0;
}

void Battle::determineStyle() {
    int stab = player_.getEquipmentBonus("attack_stab");
    int slash = player_.getEquipmentBonus("attack_slash");
    int crush = player_.getEquipmentBonus("attack_crush");
    int range = player_.getEquipmentBonus("attack_ranged");
    
    // Simple heuristic: pick highest bonus
    if (range > stab && range > slash && range > crush) {
        style_ = "ranged";
        isRanged_ = true;
    } else {
        isRanged_ = false;
        if (stab >= slash && stab >= crush) style_ = "stab";
        else if (slash >= stab && slash >= crush) style_ = "slash";
        else style_ = "crush";
    }
}

int Battle::effectiveStrength() {
    if (isRanged_) {
        // ((Ranged + Boost) * Prayer + 8 + Style)
        int rng = player_.getBoostedLevel("Ranged");
        
        // Apply Prayer (Rigour gives +23% Ranged Str)
        if (player_.isRigourActive()) {
            rng = static_cast<int>(rng * 1.23);
        }
        
        int str = rng + 8 + stance_bonus_strength_;
        
        // Void Range logic if needed (usually 10% or 12.5% for Elite)
        // I'll leave Ranged Void for now as not explicitly in conflicts.
        return str;
    }

    // ((Strength + Boost) * Prayer + 8 + Style)
    int str = player_.getBoostedLevel("Strength");
    
    // Apply Prayer (Piety gives +23% Strength)
    if (player_.isPietyActive()) {
        str = static_cast<int>(str * 1.23);
    }
    
    // Void Melee
    if (activeSet_ == "Void Melee" || activeSet_ == "Elite Void Melee") {
        str = static_cast<int>(str * 1.10);
    }
    
    return str + 8 + stance_bonus_strength_;
}

int Battle::effectiveAttack() {
    if (isRanged_) {
        int rng = player_.getBoostedLevel("Ranged");
        
        // Apply Prayer (Rigour gives +20% Ranged Attack)
        if (player_.isRigourActive()) {
            rng = static_cast<int>(rng * 1.20);
        }
        
        // Void Range could apply here too
        return rng + 8 + stance_bonus_attack_;
    }

    int att = player_.getBoostedLevel("Attack");
    
    // Apply Prayer (Piety gives +20% Attack)
    if (player_.isPietyActive()) {
        att = static_cast<int>(att * 1.20);
    }
    
    // Void Melee
    if (activeSet_ == "Void Melee" || activeSet_ == "Elite Void Melee") {
        att = static_cast<int>(att * 1.10);
    }
    
    return att + 8 + stance_bonus_attack_;
}

int Battle::maxHit() {
    int effStr = effectiveStrength();
    
    if (isRanged_) {
        // ((EffRangedStr * (RangedStrBonus + 64) + 320) / 640)
        int equipStr = player_.getEquipmentBonus("ranged_strength");
        int baseMax = ((effStr * (equipStr + 64) + 320) / 640);
        
        double multiplier = 1.0;
        
        // Salve Amulet (i) and (ei) boost Ranged
        if (isUndead_) {
            if (hasSalveEI_) multiplier *= 1.20;
            else if (hasSalveI_) multiplier *= 1.1667;
        }
        
        // Slayer Helm (i) boosts Ranged
        if (isOnTask_) {
            if (player_.hasEquipped("Slayer helmet (i)") ||
                player_.hasEquipped("Black mask (i)")) {
                 // 15% for Ranged/Magic usually
                 multiplier *= 1.15;
            }
        }
        
        // Dragon Hunter Crossbow
        if (isDragon_ && isDHCB_) {
            multiplier *= 1.30;
        }
        
        // Twisted Bow
        if (isTbow_) {
            int magic = monster_.getInt("magic_level");
            int cap = 250; 
            if (magic > cap) magic = cap;
            
            double tbowMult = 0.25 + (magic * 3 - 14) / 100.0;
            if (tbowMult > 2.5) tbowMult = 2.5;
            if (tbowMult < 1.0) tbowMult = 1.0; 
            
            multiplier *= tbowMult;
        }
        
        // Crystal Armor logic would go here
        
        return static_cast<int>(baseMax * multiplier);
    }

    // ((EffStr * (EquipStr + 64) + 320) / 640)
    // Try strength_bonus, fallback to melee_strength if 0 (heuristic)
    int equipStr = player_.getEquipmentBonus("strength_bonus");
    if (equipStr == 0) equipStr = player_.getEquipmentBonus("melee_strength");
    
    int baseMax = ((effStr * (equipStr + 64) + 320) / 640);
    
    double multiplier = 1.0;
    
    // --- Slayer Helm / Black Mask ---
    if (isOnTask_) {
        if (player_.hasEquipped("Slayer helmet") || 
            player_.hasEquipped("Slayer helmet (i)") ||
            player_.hasEquipped("Black mask") || 
            player_.hasEquipped("Black mask (i)")) {
             multiplier *= 1.1667;
        }
    }
    
    // --- Salve Amulet (Undead) ---
    if (isUndead_) {
        double salveMult = 1.0;
        if (hasSalveEI_ || hasSalveE_) salveMult = 1.20;
        else if (hasSalveI_ || hasSalve_) salveMult = 1.1667;
        
        if (salveMult > 1.0) {
            bool slayerApplied = (multiplier > 1.05); 
            if (slayerApplied) {
                multiplier /= 1.1667; 
            }
            multiplier *= salveMult;
        }
    }
    
    // --- Weapon Attributes ---
    
    if (isDragon_) {
        if (isDHL_) multiplier *= 1.20;
        // DHCB is ranged only
    }
    
    if (isDemon_) {
        if (isArclight_) multiplier *= 1.70;
    }
    
    if (isKalphite_) {
        if (isKeris_ || isKerisBreaching_) multiplier *= 1.33;
    }
    
    if (isLeafy_ && isLeafBladed_) {
        multiplier *= 1.175;
    }
    
    // --- Sets ---
    
    // Obsidian (Melee)
    if (activeSet_ == "Obsidian") {
         auto weapon = player_.getEquippedItem("weapon").getName();
         if (weapon.find("Toktz-xil") != std::string::npos || weapon.find("Tzhaar-ket") != std::string::npos) {
             multiplier *= 1.10;
         }
    }
    
    // Inquisitor (Crush)
    if (activeSet_ == "Inquisitor" && style_ == "crush") {
        multiplier *= 1.025; // 2.5% total
    } else if (style_ == "crush") {
        // Individual pieces (0.5% each)
        if (player_.getEquippedItem("head").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
        if (player_.getEquippedItem("body").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
        if (player_.getEquippedItem("legs").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
    }
    
    // --- Specials ---
    
    // Dharok
    if (isDharok_) {
        double lostHP = (double)(player_.getMaxHP() - player_.getCurrentHP());
        double hpMult = 1.0 + (lostHP / 100.0 * (player_.getMaxHP() / 100.0));
        multiplier *= hpMult;
    }

    return static_cast<int>(baseMax * multiplier);
}

int Battle::attackRoll() {
    int effAtt = effectiveAttack();
    
    if (isRanged_) {
        int equipAtt = player_.getEquipmentBonus("attack_ranged");
        int roll = effAtt * (equipAtt + 64);
        
        double multiplier = 1.0;
        if (isUndead_) {
            if (hasSalveEI_) multiplier *= 1.20;
            else if (hasSalveI_) multiplier *= 1.1667;
        }
        
        // Slayer Helm (i) Ranged Accuracy
        if (isOnTask_) {
             if (player_.hasEquipped("Slayer helmet (i)") ||
                 player_.hasEquipped("Black mask (i)")) {
                 multiplier *= 1.15;
             }
        }
        
        // DHCB Accuracy
        if (isDragon_ && isDHCB_) {
            multiplier *= 1.30;
        }

        // Twisted Bow Accuracy
        if (isTbow_) {
             int magic = monster_.getInt("magic_level");
             int cap = 250;
             if (magic > cap) magic = cap;
             // Accuracy formula: 140 + (30*magic - 10)/100 ... rough
             double tbowAcc = 1.40 + (30 * magic - 10) / 100.0;
             if (tbowAcc > 2.40) tbowAcc = 2.40; // Cap
             multiplier *= tbowAcc;
        }

        return static_cast<int>(roll * multiplier);
    }

    // EffAtt * (EquipAtt + 64)
    int equipAtt = player_.getEquipmentBonus("attack_" + style_);
    int roll = effAtt * (equipAtt + 64);
    
    double multiplier = 1.0;
    
    // --- Slayer Helm ---
    if (isOnTask_) {
        if (player_.hasEquipped("Slayer helmet") || 
            player_.hasEquipped("Slayer helmet (i)") ||
            player_.hasEquipped("Black mask") || 
            player_.hasEquipped("Black mask (i)")) {
             multiplier *= 1.1667;
        }
    }
    
    // --- Salve (Undead) ---
    if (isUndead_) {
        double salveMult = 1.0;
        if (hasSalveEI_ || hasSalveE_) salveMult = 1.20;
        else if (hasSalveI_ || hasSalve_) salveMult = 1.1667;
        
        if (salveMult > 1.0) {
             bool slayerApplied = (multiplier > 1.05);
             if (slayerApplied) multiplier /= 1.1667;
             multiplier *= salveMult;
        }
    }
    
    // --- Weapon Attributes ---
    if (isDragon_) {
        if (isDHL_) multiplier *= 1.20;
    }
    
    if (isDemon_) {
        if (isArclight_) multiplier *= 1.70;
    }
    
    if (isKalphite_ && isKerisBreaching_) {
        multiplier *= 1.33;
    }
    
    // Obsidian
    if (activeSet_ == "Obsidian") {
         auto weapon = player_.getEquippedItem("weapon").getName();
         if (weapon.find("Toktz-xil") != std::string::npos || weapon.find("Tzhaar-ket") != std::string::npos) {
             multiplier *= 1.10;
         }
    }
    
    // Inquisitor (Crush)
    if (activeSet_ == "Inquisitor" && style_ == "crush") {
        multiplier *= 1.025;
    } else if (style_ == "crush") {
        if (player_.getEquippedItem("head").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
        if (player_.getEquippedItem("body").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
        if (player_.getEquippedItem("legs").getName().find("Inquisitor") != std::string::npos) multiplier *= 1.005;
    }
    
    return static_cast<int>(roll * multiplier);
}

int Battle::defenceRoll() {
    int def = monster_.getInt("defence_level");
    int defBonus = 0;
    
    if (isRanged_) {
        defBonus = monster_.getInt("defence_ranged");
    } else {
        defBonus = monster_.getInt("defence_" + style_);
    }
    
    return (def + 9) * (defBonus + 64);
}

double Battle::hitChance() {
    int a = attackRoll();
    int d = defenceRoll();
    
    double p = 0.0;
    double A = static_cast<double>(a);
    double D = static_cast<double>(d);

    if (A > D) {
        p = 1.0 - (D + 2.0) / (2.0 * (A + 1.0));
    } else {
        p = A / (2.0 * (D + 1.0));
    }

    if (isFang_ && style_ == "stab") {
        return 1.0 - (1.0 - p) * (1.0 - p);
    }

    return p;
}

int Battle::bernoulliTrial(double p) {
    std::bernoulli_distribution d(p);
    return d(gen) ? 1 : 0;
}

int Battle::randomDamage(int max) {
    int minHit = 0;
    int maxHit = max;
    
    if (isFang_) {
        minHit = static_cast<int>(max * 0.15);
        maxHit = static_cast<int>(max * 0.85);
        if (minHit > maxHit) minHit = maxHit;
    }

    std::uniform_int_distribution<int> d(minHit, maxHit);
    return d(gen);
}

int Battle::simulate() {
    int hp = monster_.getInt("hitpoints");
    int max = maxHit();
    double chance = hitChance();
    int ticks = 0;
    
    // Scythe handling
    int size = monster_.getSize();
    int hits = 1;
    if (isScythe_) {
        if (size == 1) hits = 1;
        else if (size == 2) hits = 2;
        else hits = 3;
    }
    
    while (hp > 0) {
        ticks += attack_speed_;
        
        // Hit 1
        if (bernoulliTrial(chance)) {
            int dmg = randomDamage(max);
            // Keris crit check
            if (isKeris_ && isKalphite_) {
                 std::uniform_int_distribution<int> crit(1, 51);
                 if (crit(gen) == 1) dmg *= 3;
            }
            hp -= dmg;
        }
        
        // Scythe Hit 2 (50% dmg)
        if (isScythe_ && hits >= 2 && hp > 0) {
             if (bernoulliTrial(chance)) {
                 int dmg = randomDamage(max / 2);
                 hp -= dmg;
             }
        }
        
        // Scythe Hit 3 (25% dmg)
        if (isScythe_ && hits >= 3 && hp > 0) {
             if (bernoulliTrial(chance)) {
                 int dmg = randomDamage(max / 4);
                 hp -= dmg;
             }
        }
    }
    return ticks;
}

double Battle::calculateDPS(const std::string& style, int stanceAtt, int stanceStr) {
    std::string oldStyle = style_;
    int oldStanceAtt = stance_bonus_attack_;
    int oldStanceStr = stance_bonus_strength_;
    
    style_ = style;
    stance_bonus_attack_ = stanceAtt;
    stance_bonus_strength_ = stanceStr;
    
    int mHit = maxHit();
    double chance = hitChance();
    
    // Calculate average damage per attack sequence
    double avgDmg = 0.0;
    
    // Base Hit
    double hit1 = (double)mHit * 0.5 * chance;

    // Keris Crit
    if (isKeris_ && isKalphite_) {
        hit1 *= (53.0/51.0);
    }
    
    avgDmg += hit1;
    
    // Scythe Extra Hits
    if (isScythe_) {
        int size = monster_.getSize();
        if (size >= 2) {
            avgDmg += (double)(mHit / 2) * 0.5 * chance;
        }
        if (size >= 3) {
            avgDmg += (double)(mHit / 4) * 0.5 * chance;
        }
    }
    
    style_ = oldStyle;
    stance_bonus_attack_ = oldStanceAtt;
    stance_bonus_strength_ = oldStanceStr;
    
    double secondsPerHit = (double)attack_speed_ * 0.6;
    
    return avgDmg / secondsPerHit;
}

double Battle::solveOptimalDPS() {
    struct Option {
        std::string style;
        std::string stanceName;
        int stanceAtt;
        int stanceStr;
        double dps;
    };
    
    // Refresh state
    determineStyle();
    
    // Base speed check
    attack_speed_ = 4;
    auto gear = player_.getGear();
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed_ = speed;
    }

    std::vector<Option> options;
    if (isRanged_) {
        // Ranged Options
        // Accurate: +3 Range (Att & Str)
        options.push_back({"ranged", "Accurate (+3 Range)", 3, 3, 0.0});
        // Rapid: Speed -1, +0 stats
        options.push_back({"ranged", "Rapid (Speed -1)", 0, 0, 0.0});
        // Longrange: +3 Def (0 range stats)
        options.push_back({"ranged", "Longrange (+3 Def)", 0, 0, 0.0});
    } else {
        options = {
            {"stab", "Accurate (+3 Att)", 3, 0, 0.0},
            {"stab", "Aggressive (+3 Str)", 0, 3, 0.0},
            {"slash", "Accurate (+3 Att)", 3, 0, 0.0},
            {"slash", "Aggressive (+3 Str)", 0, 3, 0.0},
            {"crush", "Accurate (+3 Att)", 3, 0, 0.0},
            {"crush", "Aggressive (+3 Str)", 0, 3, 0.0}
        };
    }
    
    Option bestOption = options[0];
    bestOption.dps = -1.0;
    
    for (auto& opt : options) {
        int oldSpeed = attack_speed_;
        if (opt.stanceName.find("Rapid") != std::string::npos) {
            attack_speed_ -= 1;
        }
        
        opt.dps = calculateDPS(opt.style, opt.stanceAtt, opt.stanceStr);
        
        if (opt.dps > bestOption.dps) {
            bestOption = opt;
        }
        
        // Restore speed
        attack_speed_ = oldSpeed;
    }
    
    style_ = bestOption.style;
    stance_bonus_attack_ = bestOption.stanceAtt;
    stance_bonus_strength_ = bestOption.stanceStr;
    
    if (bestOption.stanceName.find("Rapid") != std::string::npos) {
        attack_speed_ -= 1;
    }
    
    return bestOption.dps;
}

void Battle::optimizeAttackStyle() {
    double dps = solveOptimalDPS();
    
    std::string stanceName = "Unknown";
    if (stance_bonus_attack_ == 3) stanceName = "Accurate (+3 Att)";
    else if (stance_bonus_strength_ == 3) stanceName = "Aggressive (+3 Str)";
}

double Battle::runSimulations(int n) {
    long totalTicks = 0;
    for (int i = 0; i < n; ++i) {
        totalTicks += simulate();
    }
    return static_cast<double>(totalTicks) / n;
}

int Battle::getMaxHit() {
    return maxHit();
}

double Battle::getHitChance() {
    return hitChance();
}

double Battle::getDPS() {
    int mHit = maxHit();
    double chance = hitChance();
    double avgDmg = 0.0;
    
    double hit1 = (double)mHit * 0.5 * chance;
    if (isKeris_ && isKalphite_) hit1 *= (53.0/51.0);
    avgDmg += hit1;
    
    if (isScythe_) {
        int size = monster_.getSize();
        if (size >= 2) avgDmg += (double)(mHit / 2) * 0.5 * chance;
        if (size >= 3) avgDmg += (double)(mHit / 4) * 0.5 * chance;
    }

    double secondsPerHit = (double)attack_speed_ * 0.6;
    return avgDmg / secondsPerHit;
}

BattleResult Battle::getResults() {
    optimizeAttackStyle();
    
    BattleResult result;
    result.dps = getDPS();
    result.maxHit = maxHit();
    result.hitChance = hitChance();
    result.style = style_;
    result.attackSpeed = attack_speed_;
    result.isFang = isFang_;
    result.isDHL = isDHL_;
    result.hasSalve = hasSalve_ || hasSalveE_ || hasSalveI_ || hasSalveEI_;
    
    result.isDHCB = isDHCB_;
    result.isArclight = isArclight_;
    result.isKeris = isKeris_;
    result.isScythe = isScythe_;
    result.isTbow = isTbow_;
    result.onTask = isOnTask_;
    result.activeSet = activeSet_;
    
    if (stance_bonus_attack_ > 0) {
        result.stance = "Accurate";
    } else if (stance_bonus_strength_ > 0) {
        result.stance = "Aggressive";
    } else {
        result.stance = "Controlled";
    }
    
    int monsterHP = monster_.getInt("hitpoints");
    if (result.dps > 0 && monsterHP > 0) {
        result.avgTTK = monsterHP / result.dps;
        result.killsPerHour = 3600.0 / result.avgTTK;
    } else {
        result.avgTTK = 0;
        result.killsPerHour = 0;
    }
    
    return result;
}
