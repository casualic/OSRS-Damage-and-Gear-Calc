#include "battle.h"
#include <iostream>
#include <numeric>
#include <vector>

Battle::Battle(Player& p, Monster& m) : player_(p), monster_(m) {
    std::random_device rd;
    gen = std::mt19937(rd());
    
    // Determine speed first
    attack_speed_ = 4; // Default Unarmed
    auto gear = player_.getGear();
    if (gear.count("weapon")) {
        Item weapon = gear.at("weapon");
        int speed = weapon.getInt("attack_speed");
        if (speed > 0) attack_speed_ = speed;
    }

    determineStyle();
}

void Battle::determineStyle() {
    int stab = player_.getEquipmentBonus("attack_stab");
    int slash = player_.getEquipmentBonus("attack_slash");
    int crush = player_.getEquipmentBonus("attack_crush");
    
    // Simple heuristic: pick highest bonus
    if (stab >= slash && stab >= crush) style_ = "stab";
    else if (slash >= stab && slash >= crush) style_ = "slash";
    else style_ = "crush";
    
    // std::cout << "Auto-detected style: " << style_ << "\n";
}

int Battle::effectiveStrength() {
    // ((Strength + Boost) * Prayer + 8)
    int str = player_.getEffectiveStat("Strength"); 
    return str + 8;
}

int Battle::maxHit() {
    // ((EffStr * (EquipStr + 64) + 320) / 640)
    int effStr = effectiveStrength();
    // Try strength_bonus, fallback to melee_strength if 0 (heuristic)
    int equipStr = player_.getEquipmentBonus("strength_bonus");
    if (equipStr == 0) equipStr = player_.getEquipmentBonus("melee_strength");
    
    return ((effStr * (equipStr + 64) + 320) / 640);
}

int Battle::effectiveAttack() {
    // ((Attack + Boost) * Prayer + 8)
    int att = player_.getEffectiveStat("Attack");
    return att + 8;
}

int Battle::attackRoll() {
    // EffAtt * (EquipAtt + 64)
    int effAtt = effectiveAttack();
    int equipAtt = player_.getEquipmentBonus("attack_" + style_);
    return effAtt * (equipAtt + 64);
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
    
    if (a > d) {
        return 1.0 - (static_cast<double>(d) + 2.0) / (2.0 * (static_cast<double>(a) + 1.0));
    } else {
        return static_cast<double>(a) / (2.0 * (static_cast<double>(d) + 1.0));
    }
}

int Battle::bernoulliTrial(double p) {
    std::bernoulli_distribution d(p);
    return d(gen) ? 1 : 0;
}

int Battle::randomDamage(int max) {
    std::uniform_int_distribution<int> d(0, max);
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

void Battle::runSimulations(int n) {
    long totalTicks = 0;
    for (int i=0; i<n; ++i) {
        totalTicks += simulate();
    }
    double avgTicks = static_cast<double>(totalTicks) / n;
    double avgSeconds = avgTicks * 0.6;
    
    std::cout << "\n=== Battle Simulation (" << n << " runs) ===\n";
    std::cout << "Player: " << player_.getEffectiveStat("Overall") << " (Placeholder name logic)\n"; 
    
    std::cout << "Target: " << monster_.getStr("name") << " (HP: " << monster_.getInt("hitpoints") << ")\n";
    std::cout << "Combat Style: " << style_ << " | Attack Speed: " << attack_speed_ << " ticks\n";
    std::cout << "Max Hit: " << maxHit() << "\n";
    std::cout << "Hit Chance: " << (hitChance() * 100.0) << "%\n";
    std::cout << "Avg TTK: " << avgSeconds << "s (" << avgTicks << " ticks)\n";
    std::cout << "DPS: " << (monster_.getInt("hitpoints") / avgSeconds) << "\n";
    std::cout << "=======================================\n";
}
