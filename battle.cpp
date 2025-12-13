// battle.cpp
#include "battle.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <iomanip>

// Helper to initialize battle from player/monster
void initBattle(Battle* b, Player& p, Monster& m, std::mt19937& gen,
                std::string& style, int& attack_speed,
                bool& isFang, bool& isDHL, bool& isDragon, bool& isUndead,
                bool& hasSalve, bool& hasSalveE, bool& hasSalveI, bool& hasSalveEI,
                int& stance_bonus_attack, int& stance_bonus_strength) {
    std::random_device rd;
    gen = std::mt19937(rd());
    
    attack_speed = 4;
    isFang = false;
    isDHL = false;
    hasSalve = false;
    hasSalveE = false;
    hasSalveI = false;
    hasSalveEI = false;
    
    isDragon = m.hasAttribute("dragon");
    isUndead = m.hasAttribute("undead");

    auto gear = p.getGear();
    
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed = speed;
        
        std::string name = weapon.getName();
        if (name.find("Osmumten's fang") != std::string::npos) isFang = true;
        if (name.find("Dragon hunter lance") != std::string::npos) isDHL = true;
    }
    
    if (gear.count("neck")) {
        std::string neck = gear.at("neck").getName();
        if (neck.find("Salve amulet") != std::string::npos) {
            if (neck.find("(ei)") != std::string::npos) hasSalveEI = true;
            else if (neck.find("(i)") != std::string::npos) hasSalveI = true;
            else if (neck.find("(e)") != std::string::npos) hasSalveE = true;
            else hasSalve = true;
        }
    }

    stance_bonus_attack = 3;
    stance_bonus_strength = 0;
}

Battle::Battle(Player& p, Monster& m) : player_(p), monster_(m) {
    initBattle(this, player_, monster_, gen, style_, attack_speed_,
               isFang_, isDHL_, isDragon_, isUndead_,
               hasSalve_, hasSalveE_, hasSalveI_, hasSalveEI_,
               stance_bonus_attack_, stance_bonus_strength_);
    determineStyle();
}

Battle::Battle(const Player& p, const Monster& m) : player_(p), monster_(m) {
    initBattle(this, player_, monster_, gen, style_, attack_speed_,
               isFang_, isDHL_, isDragon_, isUndead_,
               hasSalve_, hasSalveE_, hasSalveI_, hasSalveEI_,
               stance_bonus_attack_, stance_bonus_strength_);
    determineStyle();
}

void Battle::determineStyle() {
    int stab = player_.getEquipmentBonus("attack_stab");
    int slash = player_.getEquipmentBonus("attack_slash");
    int crush = player_.getEquipmentBonus("attack_crush");
    
    if (stab >= slash && stab >= crush) style_ = "stab";
    else if (slash >= stab && slash >= crush) style_ = "slash";
    else style_ = "crush";
}

int Battle::effectiveStrength() {
    int str = player_.getEffectiveStat("Strength"); 
    return str + 8 + stance_bonus_strength_;
}

int Battle::maxHit() {
    int effStr = effectiveStrength();
    int equipStr = player_.getEquipmentBonus("strength_bonus");
    if (equipStr == 0) equipStr = player_.getEquipmentBonus("melee_strength");
    
    int baseMax = ((effStr * (equipStr + 64) + 320) / 640);
    
    double multiplier = 1.0;
    
    if (isDHL_ && isDragon_) {
        multiplier *= 1.20;
    }
    
    if (isUndead_) {
        if (hasSalveEI_) multiplier *= 1.20;
        else if (hasSalveE_) multiplier *= 1.20;
        else if (hasSalveI_) multiplier *= 1.1667;
        else if (hasSalve_) multiplier *= 1.1667;
    }
    
    return static_cast<int>(baseMax * multiplier);
}

int Battle::effectiveAttack() {
    int att = player_.getEffectiveStat("Attack");
    return att + 8 + stance_bonus_attack_;
}

int Battle::attackRoll() {
    int effAtt = effectiveAttack();
    int equipAtt = player_.getEquipmentBonus("attack_" + style_);
    int roll = effAtt * (equipAtt + 64);
    
    double multiplier = 1.0;
    
    if (isDHL_ && isDragon_) {
        multiplier *= 1.20;
    }
    
    if (isUndead_) {
        if (hasSalveEI_) multiplier *= 1.20;
        else if (hasSalveE_) multiplier *= 1.20; 
        else if (hasSalveI_) multiplier *= 1.1667; 
        else if (hasSalve_) multiplier *= 1.1667; 
    }
    
    return static_cast<int>(roll * multiplier);
}

int Battle::defenceRoll() {
    int def = monster_.getInt("defence_level");
    int defBonus = monster_.getInt("defence_" + style_);
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
    
    while (hp > 0) {
        ticks += attack_speed_;
        if (bernoulliTrial(chance)) {
            int dmg = randomDamage(max);
            hp -= dmg;
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
    
    style_ = oldStyle;
    stance_bonus_attack_ = oldStanceAtt;
    stance_bonus_strength_ = oldStanceStr;
    
    double avgDmgPerHit = (double)mHit * 0.5 * chance;
    double secondsPerHit = (double)attack_speed_ * 0.6;
    
    return avgDmgPerHit / secondsPerHit;
}

double Battle::solveOptimalDPS() {
    struct Option {
        std::string style;
        std::string stanceName;
        int stanceAtt;
        int stanceStr;
        double dps;
    };
    
    std::vector<Option> options = {
        {"stab", "Accurate (+3 Att)", 3, 0, 0.0},
        {"stab", "Aggressive (+3 Str)", 0, 3, 0.0},
        {"slash", "Accurate (+3 Att)", 3, 0, 0.0},
        {"slash", "Aggressive (+3 Str)", 0, 3, 0.0},
        {"crush", "Accurate (+3 Att)", 3, 0, 0.0},
        {"crush", "Aggressive (+3 Str)", 0, 3, 0.0}
    };
    
    Option bestOption = options[0];
    bestOption.dps = -1.0;
    
    attack_speed_ = 4;
    auto gear = player_.getGear();
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed_ = speed;
    }

    for (auto& opt : options) {
        opt.dps = calculateDPS(opt.style, opt.stanceAtt, opt.stanceStr);
        if (opt.dps > bestOption.dps) {
            bestOption = opt;
        }
    }
    
    style_ = bestOption.style;
    stance_bonus_attack_ = bestOption.stanceAtt;
    stance_bonus_strength_ = bestOption.stanceStr;
    
    return bestOption.dps;
}

void Battle::optimizeAttackStyle() {
    std::cout << "\n--- Optimizing Attack Style ---\n";
    double dps = solveOptimalDPS();
    
    std::string stanceName = "Unknown";
    if (stance_bonus_attack_ == 3) stanceName = "Accurate (+3 Att)";
    else if (stance_bonus_strength_ == 3) stanceName = "Aggressive (+3 Str)";
    
    std::cout << ">>> Selected Best: " << style_ << " (" << stanceName << ") | DPS: " << dps << "\n";
}

double Battle::runSimulations(int n) {
    long totalTicks = 0;
    for (int i = 0; i < n; ++i) {
        totalTicks += simulate();
    }
    return static_cast<double>(totalTicks) / n;
}

// New getter methods for UI
int Battle::getMaxHit() {
    return maxHit();
}

double Battle::getHitChance() {
    return hitChance();
}

double Battle::getDPS() {
    int mHit = maxHit();
    double chance = hitChance();
    double avgDmgPerHit = (double)mHit * 0.5 * chance;
    double secondsPerHit = (double)attack_speed_ * 0.6;
    return avgDmgPerHit / secondsPerHit;
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
