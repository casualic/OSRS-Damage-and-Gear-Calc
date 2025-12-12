#include "battle.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <iomanip>

Battle::Battle(Player& p, Monster& m) : player_(p), monster_(m) {
    std::random_device rd;
    gen = std::mt19937(rd());
    
    // Determine speed first
    attack_speed_ = 4; // Default Unarmed
    isFang_ = false;
    isDHL_ = false;
    hasSalve_ = false;
    hasSalveE_ = false;
    hasSalveI_ = false;
    hasSalveEI_ = false;
    
    // Check Monster Attributes
    isDragon_ = monster_.hasAttribute("dragon");
    isUndead_ = monster_.hasAttribute("undead");

    auto gear = player_.getGear();
    
    // Check Weapon
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed_ = speed;
        
        std::string name = weapon.getName();
        if (name.find("Osmumten's fang") != std::string::npos) isFang_ = true;
        if (name.find("Dragon hunter lance") != std::string::npos) isDHL_ = true;
    }
    
    // Check Neck (Salve)
    if (gear.count("neck")) {
        std::string neck = gear.at("neck").getName();
        if (neck.find("Salve amulet") != std::string::npos) {
            if (neck.find("(ei)") != std::string::npos) hasSalveEI_ = true;
            else if (neck.find("(i)") != std::string::npos) hasSalveI_ = true;
            else if (neck.find("(e)") != std::string::npos) hasSalveE_ = true;
            else hasSalve_ = true;
        }
    }

    determineStyle(); // Initial guess
    
    // For now, defaulting to Accurate (+3 Attack)
    stance_bonus_attack_ = 3;
    stance_bonus_strength_ = 0;
}

void Battle::determineStyle() {
    int stab = player_.getEquipmentBonus("attack_stab");
    int slash = player_.getEquipmentBonus("attack_slash");
    int crush = player_.getEquipmentBonus("attack_crush");
    
    // Simple heuristic: pick highest bonus
    if (stab >= slash && stab >= crush) style_ = "stab";
    else if (slash >= stab && slash >= crush) style_ = "slash";
    else style_ = "crush";
}

int Battle::effectiveStrength() {
    // ((Strength + Boost) * Prayer + 8 + Style)
    int str = player_.getEffectiveStat("Strength"); 
    return str + 8 + stance_bonus_strength_;
}

int Battle::maxHit() {
    // ((EffStr * (EquipStr + 64) + 320) / 640)
    int effStr = effectiveStrength();
    // Try strength_bonus, fallback to melee_strength if 0 (heuristic)
    int equipStr = player_.getEquipmentBonus("strength_bonus");
    if (equipStr == 0) equipStr = player_.getEquipmentBonus("melee_strength");
    
    int baseMax = ((effStr * (equipStr + 64) + 320) / 640);
    
    // Apply Multipliers
    double multiplier = 1.0;
    
    // Dragon Hunter Lance: +20% Damage vs Dragons
    if (isDHL_ && isDragon_) {
        multiplier *= 1.20;
    }
    
    // Salve Amulet: vs Undead
    if (isUndead_) {
        if (hasSalveEI_) multiplier *= 1.20;
        else if (hasSalveE_) multiplier *= 1.20; // Salve (e) gives 20% to melee
        else if (hasSalveI_) multiplier *= 1.1667; // Salve (i) gives 16.67%
        else if (hasSalve_) multiplier *= 1.1667; // Salve gives 16.67%
    }
    
    // Note: Salve and Slayer Helm generally don't stack (except specific cases).
    // Assuming if Salve is equipped, user wants to use it.
    
    return static_cast<int>(baseMax * multiplier);
}

int Battle::effectiveAttack() {
    // ((Attack + Boost) * Prayer + 8 + Style)
    int att = player_.getEffectiveStat("Attack");
    return att + 8 + stance_bonus_attack_;
}

int Battle::attackRoll() {
    // EffAtt * (EquipAtt + 64)
    int effAtt = effectiveAttack();
    int equipAtt = player_.getEquipmentBonus("attack_" + style_);
    int roll = effAtt * (equipAtt + 64);
    
    // Apply Multipliers
    double multiplier = 1.0;
    
    // Dragon Hunter Lance: +20% Accuracy vs Dragons
    if (isDHL_ && isDragon_) {
        multiplier *= 1.20;
    }
    
    // Salve Amulet: vs Undead
    if (isUndead_) {
        if (hasSalveEI_) multiplier *= 1.20;
        else if (hasSalveE_) multiplier *= 1.20; 
        else if (hasSalveI_) multiplier *= 1.1667; 
        else if (hasSalve_) multiplier *= 1.1667; 
    }
    
    return static_cast<int>(roll * multiplier);
}

int Battle::defenceRoll() {
    // (TargetDef + 9) * (TargetDefBonus + 64)
    int def = monster_.getInt("defence_level");
    int defBonus = monster_.getInt("defence_" + style_);
    return (def + 9) * (defBonus + 64);
}

double Battle::hitChance() {
    int a = attackRoll();
    int d = defenceRoll();
    
    // 1. Calculate Standard Probability 'p'
    double p = 0.0;
    double A = static_cast<double>(a);
    double D = static_cast<double>(d);

    if (A > D) {
        p = 1.0 - (D + 2.0) / (2.0 * (A + 1.0));
    } else {
        p = A / (2.0 * (D + 1.0));
    }

    // 2. Apply Osmumten's Fang Passive (Double Roll on Stab)
    // "Two independent accuracy rolls. If either succeeds, the hit lands."
    if (isFang_ && style_ == "stab") {
        // P(at least one hit) = 1 - P(miss both)
        // P(miss) = 1 - p
        // P(miss both) = (1 - p)^2
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
    
    // Osmumten's Fang Passive: Damage Clamping (Always active)
    if (isFang_) {
        minHit = static_cast<int>(max * 0.15);
        maxHit = static_cast<int>(max * 0.85);
        
        // Safety: ensure min <= max
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
    // Temporarily apply settings
    std::string oldStyle = style_;
    int oldStanceAtt = stance_bonus_attack_;
    int oldStanceStr = stance_bonus_strength_;
    
    style_ = style;
    stance_bonus_attack_ = stanceAtt;
    stance_bonus_strength_ = stanceStr;
    
    // Calc logic
    int mHit = maxHit();
    double chance = hitChance();
    
    // Restore settings
    style_ = oldStyle;
    stance_bonus_attack_ = oldStanceAtt;
    stance_bonus_strength_ = oldStanceStr;
    
    // Average damage per hit = MaxHit * 0.5 * HitChance
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
    
    // Check weapon speed (it might change if weapon changed)
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
    
    // Apply best
    style_ = bestOption.style;
    stance_bonus_attack_ = bestOption.stanceAtt;
    stance_bonus_strength_ = bestOption.stanceStr;
    
    return bestOption.dps;
}

void Battle::optimizeAttackStyle() {
    std::cout << "\n--- Optimizing Attack Style ---\n";
    double dps = solveOptimalDPS();
    
    // Reverse lookup for nice printing (optional, or just print current state)
    std::string stanceName = "Unknown";
    if (stance_bonus_attack_ == 3) stanceName = "Accurate (+3 Att)";
    else if (stance_bonus_strength_ == 3) stanceName = "Aggressive (+3 Str)";
    
    std::cout << ">>> Selected Best: " << style_ << " (" << stanceName << ") | DPS: " << dps << "\n";
}

void Battle::runSimulations(int n) {
    long totalTicks = 0;
    for (int i=0; i<n; ++i) {
        totalTicks += simulate();
    }
    double avgTicks = static_cast<double>(totalTicks) / n;
    double avgSeconds = avgTicks * 0.6;
    
    std::cout << "\n=== Battle Simulation (" << n << " runs) ===\n";
    std::cout << "Player: " << player_.getEffectiveStat("Overall") << "\n"; 
    
    std::cout << "Target: " << monster_.getStr("name") << " (HP: " << monster_.getInt("hitpoints") << ")\n";
    std::cout << "Combat Style: " << style_ << " | Attack Speed: " << attack_speed_ << " ticks\n";
    std::cout << "Max Hit: " << maxHit() << "\n";
    std::cout << "Hit Chance: " << (hitChance() * 100.0) << "%\n";
    std::cout << "Avg TTK: " << avgSeconds << "s (" << avgTicks << " ticks)\n";
    std::cout << "DPS: " << (monster_.getInt("hitpoints") / avgSeconds) << "\n";
    std::cout << "=======================================\n";
}

