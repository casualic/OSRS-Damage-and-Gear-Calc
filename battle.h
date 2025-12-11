#pragma once
#include "player.h"
#include "monster.h"
#include <random>
#include <string>

class Battle {
    private:
        Player& player_;
        Monster& monster_;
        std::mt19937 gen;
        
        std::string style_; // "stab", "slash", "crush"
        int attack_speed_;

        // Formulas
        int effectiveStrength();
        int maxHit();
        int effectiveAttack();
        int attackRoll();
        int defenceRoll();
        double hitChance();
        
        int bernoulliTrial(double p);
        int randomDamage(int max);

        void determineStyle();

    public:
        Battle(Player& p, Monster& m);
        
        // Returns ticks to kill
        int simulate(); 
        
        // Runs n simulations and prints average TTK
        void runSimulations(int n);
};
