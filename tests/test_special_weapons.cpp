// test/test_special_weapons.cpp
#include "battle.h"
#include "player.h"
#include "monster.h"
#include <iostream>
#include <cassert>
#include <cmath>

void testDragonbane() {
    std::cout << "Testing Dragonbane...\n";
    Player p("TestPlayer");
    // Stats
    p.setStat("Attack", 99);
    p.setStat("Strength", 99);
    
    // Gear: DHL
    Item dhl("Dragon hunter lance");
    dhl.setInt("attack_stab", 85);
    dhl.setInt("strength_bonus", 70);
    dhl.setInt("attack_speed", 4);
    p.equip("weapon", dhl);
    
    // Monster: Vorkath (Dragon)
    Monster m("Vorkath");
    m.setInt("hitpoints", 750);
    m.setInt("defence_level", 214);
    m.setInt("defence_stab", 26);
    m.addAttribute("dragon");
    m.addAttribute("undead");
    
    Battle b(p, m);
    int maxHit = b.getMaxHit();
    
    auto res = b.getResults();
    assert(res.isDHL == true);
    
    std::cout << "DHL Max Hit: " << maxHit << "\n";
    
    // Compare with non-dragon
    Monster m2("Cow");
    m2.setInt("hitpoints", 10);
    Battle b2(p, m2);
    int maxHitNormal = b2.getMaxHit();
    std::cout << "Normal Max Hit: " << maxHitNormal << "\n";
    
    assert(maxHit > maxHitNormal);
    std::cout << "PASS\n";
}

void testSlayerHelm() {
    std::cout << "Testing Slayer Helm...\n";
    Player p("TestPlayer");
    p.setStat("Attack", 99);
    p.setStat("Strength", 99);
    
    Item whip("Abyssal whip");
    whip.setInt("strength_bonus", 82);
    whip.setInt("attack_speed", 4);
    p.equip("weapon", whip);
    
    Item helm("Slayer helmet (i)");
    helm.setInt("strength_bonus", 0); 
    p.equip("head", helm);
    
    Monster m("Abyssal Demon");
    m.setInt("hitpoints", 150);
    
    // Off Task
    p.setSlayerTask(false);
    Battle b1(p, m);
    int max1 = b1.getMaxHit();
    
    // On Task
    p.setSlayerTask(true);
    Battle b2(p, m);
    int max2 = b2.getMaxHit();
    
    std::cout << "Off Task: " << max1 << ", On Task: " << max2 << "\n";
    assert(max2 > max1);
    
    double ratio = (double)max2 / max1;
    std::cout << "Ratio: " << ratio << "\n";
    assert(ratio > 1.15 && ratio < 1.18);
    std::cout << "PASS\n";
}

void testVoid() {
    std::cout << "Testing Void Melee...\n";
    Player p("TestPlayer");
    p.setStat("Attack", 99);
    p.setStat("Strength", 99);
    
    Item whip("Abyssal whip");
    whip.setInt("strength_bonus", 82);
    whip.setInt("attack_speed", 4);
    p.equip("weapon", whip);
    
    // Full Void
    p.equip("head", Item("Void melee helm"));
    p.equip("body", Item("Void knight top"));
    p.equip("legs", Item("Void knight robe"));
    p.equip("hands", Item("Void knight gloves"));
    
    assert(p.getActiveSet() == "Void Melee");
    
    Monster m("Dummy");
    Battle b(p, m);
    int maxVoid = b.getMaxHit();
    
    // Remove gloves
    p.unequip("hands");
    assert(p.getActiveSet() == "");
    
    Battle b2(p, m);
    int maxNoVoid = b2.getMaxHit();
    
    std::cout << "Void: " << maxVoid << ", No Void: " << maxNoVoid << "\n";
    assert(maxVoid > maxNoVoid);
    std::cout << "PASS\n";
}

void testScythe() {
    std::cout << "Testing Scythe...\n";
    Player p("TestPlayer");
    p.setStat("Attack", 99);
    p.setStat("Strength", 99);
    
    Item scythe("Scythe of vitur");
    scythe.setInt("strength_bonus", 75);
    scythe.setInt("attack_slash", 110);
    scythe.setInt("attack_speed", 5);
    p.equip("weapon", scythe);
    
    // Small Monster (Size 1)
    Monster m1("Small Rat");
    m1.setSize(1);
    m1.setInt("hitpoints", 100);
    Battle b1(p, m1);
    double dps1 = b1.getDPS();
    
    // Big Monster (Size 3)
    Monster m3("Big Boss");
    m3.setSize(3);
    m3.setInt("hitpoints", 1000);
    Battle b3(p, m3);
    double dps3 = b3.getDPS();
    
    std::cout << "Size 1 DPS: " << dps1 << ", Size 3 DPS: " << dps3 << "\n";
    
    assert(dps3 > dps1);
    
    std::cout << "PASS\n";
}

void testDharok() {
    std::cout << "Testing Dharok...\n";
    Player p("TestPlayer");
    p.setStat("Hitpoints", 99);
    p.setStat("Strength", 99);
    
    Item axe("Dharok's greataxe");
    axe.setInt("strength_bonus", 105);
    axe.setInt("attack_speed", 7);
    p.equip("weapon", axe);
    p.equip("head", Item("Dharok's helm"));
    p.equip("body", Item("Dharok's platebody"));
    p.equip("legs", Item("Dharok's platelegs"));
    
    assert(p.getActiveSet() == "Dharok");
    
    Monster m("Dummy");
    m.setInt("hitpoints", 100);
    
    // Full HP
    p.setHP(99, 99);
    Battle b1(p, m);
    int maxFull = b1.getMaxHit();
    
    // 1 HP
    p.setHP(1, 99);
    Battle b2(p, m);
    int maxLow = b2.getMaxHit();
    
    std::cout << "Full HP Max: " << maxFull << ", 1 HP Max: " << maxLow << "\n";
    assert(maxLow > maxFull);
    std::cout << "PASS\n";
}

void testBolts() {
    std::cout << "Testing Ruby Bolts...\n";
    Player p("TestPlayer");
    p.setStat("Ranged", 99);
    
    Item rcb("Rune crossbow");
    rcb.setStr("weapon_type", "crossbow");
    rcb.setInt("attack_speed", 5); 
    rcb.setInt("ranged_strength", 90); 
    rcb.setInt("attack_ranged", 90);
    p.equip("weapon", rcb);
    
    Item bolts("Ruby bolts (e)");
    bolts.setInt("ranged_strength", 49);
    p.equip("ammo", bolts);
    
    Monster m("Big Boss");
    m.setInt("hitpoints", 500);
    m.setInt("defence_level", 50); 
    
    Battle b(p, m);
    double dps1 = b.getDPS();
    
    // Test Zaryte
    Item zcb("Zaryte crossbow");
    zcb.setStr("weapon_type", "crossbow");
    zcb.setInt("attack_speed", 5);
    zcb.setInt("ranged_strength", 90);
    zcb.setInt("attack_ranged", 90);
    p.equip("weapon", zcb);
    
    Battle b2(p, m);
    double dps2 = b2.getDPS();
    
    std::cout << "RCB DPS: " << dps1 << ", ZCB DPS: " << dps2 << "\n";
    assert(dps2 > dps1); // Zaryte should boost ruby damage/effect
    std::cout << "PASS\n";
}

int main() {
    testDragonbane();
    testSlayerHelm();
    testVoid();
    testScythe();
    testDharok();
    testBolts();
    
    std::cout << "All tests passed!\n";
    return 0;
}
