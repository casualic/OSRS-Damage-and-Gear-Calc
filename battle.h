#pragma once
#include "player.h"
#include "monster.h"
#include <random>
#include <string>

struct BattleResult {
    double dps;
    int maxHit;
    double hitChance;
    std::string style;
    std::string stance;
    int attackSpeed;
    double avgTTK;
    double killsPerHour;
    bool isFang;
    bool isDHL;
    bool hasSalve;
};

class Battle {
    private:
        Player player_;  // Copy for WASM compatibility
        Monster monster_;
        std::mt19937 gen;
        
        std::string style_; // "stab", "slash", "crush"
        int attack_speed_;
        bool isFang_ {false};
        bool isDHL_ {false};
        bool isDragon_ {false};
        bool isUndead_ {false};
        bool hasSalve_ {false};
        bool hasSalveE_ {false};
        bool hasSalveI_ {false};
        bool hasSalveEI_ {false};
        
        int stance_bonus_attack_ {0};
        int stance_bonus_strength_ {0};

        // Formulas
        int effectiveStrength();
        int maxHit();
        int effectiveAttack();
        int attackRoll();
        int defenceRoll();
        double hitChance();
        
        int bernoulliTrial(double p);
        int randomDamage(int max);

        void determineStyle(); // Default logic
        
        // Helper to calculate theoretical DPS for a given style/stance
        double calculateDPS(const std::string& style, int stanceAtt, int stanceStr);

    public:
        Battle(Player& p, Monster& m);
        Battle(const Player& p, const Monster& m);
        
        // Returns ticks to kill
        int simulate(); 
        
        // Runs n simulations and returns avg ticks
        double runSimulations(int n);
        
        // Tests styles and sets the best one
        void optimizeAttackStyle(); 
        
        // Returns the max theoretical DPS achievable with current gear
        double solveOptimalDPS();
        
        // Getters for UI
        std::string getStyle() const { return style_; }
        int getAttackSpeed() const { return attack_speed_; }
        int getMaxHit();
        double getHitChance();
        double getDPS();
        BattleResult getResults();
        
        // Special effect indicators
        bool hasFang() const { return isFang_; }
        bool hasDHL() const { return isDHL_; }
        bool hasAnySalve() const { return hasSalve_ || hasSalveE_ || hasSalveI_ || hasSalveEI_; }
};

