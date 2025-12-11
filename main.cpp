#include <iostream>
#include <map>
#include <stdexcept>
#include <random>
#include <fstream>
#include "player.h"
#include "monster.h"
#include "json.hpp"
#include "item.h"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


struct CombatStats{
    int attack;
    int strength;
    int defence;
    int ranged;
    int magic;
    int prayer;

};

std::map<std::string, int> enemyTypes = {
    {"NPC", 1},
    {"Player", 3}
};

int effectiveStrength(const CombatStats& stats,
    const std::string enemy_type,
    double strength_boost,
    double prayer_modifier,
    double void_modifier)// 'NPC' or 'Player', for now implement player only
{



    int effective_strength{};
    effective_strength = (((stats.strength +strength_boost) * prayer_modifier)
     + enemyTypes[enemy_type] + 8) * void_modifier;

    
    return effective_strength;

};


int maxHit(int effective_strength,
     int equipment_strength_bonus,
     double target_specific_gear_modifier)// for example 7/6 for slayer helm, salve vs undead 1.2
     {
        // for now PVP prayer effects are ignored.
    int max_hit {};
    max_hit = ((effective_strength * (equipment_strength_bonus +64) + 320) / 640 )
     * target_specific_gear_modifier;

    return max_hit;
};

int calculateEffectiveAttack(const CombatStats& stats,
     int attack_level_boost,
     double prayer_modifier,
     double void_modifier,
     const std::string enemy_type){

    // int attack {stats.attack};
    int effective_attack { };
    effective_attack = ((stats.attack + attack_level_boost) * prayer_modifier + enemyTypes[enemy_type]
                        + 8 ) * void_modifier;
    

    return effective_attack;
};

int attackRoll(int effective_attack,
    int equipment_attack_bonus,
    int target_specific_gear_bonus){

    int attack_roll { effective_attack * (equipment_attack_bonus +64) };

    return attack_roll;

};

void calculateEffectiveDefence(const CombatStats& stats, 
    int defence_boost, int prayer_modifier,const std::string stance){// void for now as it is a pvp func

    std::map<std::string, int> stances = {
        {"Defensive", 3 },
        {"Controlled", 1}
    };

    int effective_defence {(stats.defence + defence_boost)*prayer_modifier + stances[stance] +8 };

};

int defenceRoll(int target_defence_level, int target_style_defence_bonus, const std::string &enemy_type){

    if (enemy_type == "Player"){
        throw std::invalid_argument("Computations for PvP not supported yet \n");
    }

    int defence_roll {(target_defence_level + 9) *(target_style_defence_bonus + 64)};

    return defence_roll;

};

double hitChance(int attack_roll, int defence_roll){
    double hit_chance {};
    double d = static_cast<double>(defence_roll);
    double a = static_cast<double>(attack_roll);

    if (attack_roll > defence_roll){
        hit_chance = 1 - ((d + 2) / (2 * (a + 1)));
    } else {
        hit_chance = a / (2*(d + 1));
    }

    return hit_chance;
}




std::random_device rd;// gets a random seed from the device
std::mt19937 gen(rd());// Create a random number generator seended with rd 


int bernoulliTrial(double hit_chance){

    double p {};
    std::bernoulli_distribution dist(hit_chance);

    return dist(gen) ? 1 : 0;
}

int randomTrial(int min_range, int max_range){

    std::uniform_int_distribution<int> distr(min_range, max_range);
    return distr(gen);
}

int damageRoll(double hit_chance, int max_hit){

    int hit_success { bernoulliTrial(hit_chance) };
    int damage_roll {randomTrial(0,max_hit)};
    int damage {hit_success * damage_roll};
    
    return damage;
}


int main(){
    std::cout << "Running main class" << std::endl;

    namespace beast = boost::beast;
    namespace websocket = beast::websocket;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    //4 . Resolver hostane or ip[];
    auto results = resolver.resolve("localhost", "37767");

    //5. TCP connect
    net::connect(ws.next_layer(), results);

    //6. Websocket handshae (HTTP upgrade)
    bool connected { false };
    for (int port = 37767; port <= 37776; port++){
        try{
            auto results = resolver.resolve("localhost", std::to_string(port));
            net::connect(ws.next_layer(), results);
            connected = true;
            std::cout << "Connected to WikiSync on :" << port << std::endl;
            break;
        } catch (...) {
            continue;
        }
    }

    if (!connected) {
        std::cerr << "Could not connect to WikiSync" << std::endl;
    }

    std::cout << "finished running main class" << std::endl;



    return 0;



}


/*
int main (){

    Item test_item {"Bones"};
    int fetched_price {test_item.fetchPrice()};
    std::cout << "Sample price is : "<< fetched_price << std::endl;

    std::cout << " price from attribute is: " << test_item.getPrice() << std::endl;

    Item test_weapon {"Rune longsword"};
    test_weapon.fetchStats("items-complete.json");

    int test_stab {test_weapon.getInt("attack_stab")};

    std::cout << "Test stab statistic is" << test_stab << std::endl;
    // lets test attack_speed and weapon_type

    std::cout << "Test attack_speed is : " << test_weapon.getInt("attack_speed") << std::endl;
    std::cout << "Test weapon_type is : " << test_weapon.getStr("weapon_type") << std::endl;


    return 0;
}
*/
/*
int main(){

    CombatStats wolpistats = {91, 92, 93, 90, 85, 75};

    std::cout << "My attack stats are " << wolpistats.attack;

    int x = wolpistats.attack * 1.5;
    std::cout << "My TEST modified attack stat is " << x << '\n';
    

    int wolpi_effective_strength { effectiveStrength(wolpistats, "NPC", 0, 1,1) };

    std::cout << "My base effective strength is "<< wolpi_effective_strength << "\n";

    int wolpi_max_hit { maxHit(wolpi_effective_strength,0,1) };

    std::cout << "Raw max hit with base fists is " << wolpi_max_hit << '\n';

    int effective_attack {calculateEffectiveAttack(wolpistats,0,1,1,"NPC")};

    std::cout << "Effective attack is  " << effective_attack << '\n';

    int attack_roll{attackRoll(effective_attack,10,10)};

    std::cout << "Example attack roll is " << attack_roll << '\n';

    int defence_roll {defenceRoll(60,60,"NPC")};

    std::cout << "Defence Roll example is : " << defence_roll << '\n';

    double hit_chance {hitChance(attack_roll,defence_roll)};

    std::cout << "Hitchance is : " << hit_chance << '\n';

    int bernoulli_sample {bernoulliTrial(hit_chance)};

    std::cout << "Bernoulli sample is " << bernoulli_sample << '\n';

    int hit_value {randomTrial(0,wolpi_max_hit)};

    std::cout << "A rand integer in the max hit range is " << hit_value <<'\n';

    int dmg_dealt {damageRoll(hit_chance,wolpi_max_hit)};

    std::cout << " A random hit independent (of the above) hit is:  " << dmg_dealt << '\n';

    std::cout << " Pritnintg a set of random damage rolls; \n";

    for (int i = 0; i <5; i++){
        std::cout << damageRoll(hit_chance,wolpi_max_hit) << " |";

    }
    std::cout << '\n';

    Player myplayer{"wolpixd"};

    std::string stats { };
    stats = myplayer.fetchStats();
    // std::map<std::string, int> { parseCSV(stats) };
    myplayer.parseStats(stats);
    std::cout << "Attack skill is" << myplayer.getStat("Attack");
    
    std::string test_skill {"Magic"};
    std::cout << "Test skill 2 is" << myplayer.getStat(test_skill) << std::endl;

    std::cout << "Now testing monsters \n";

    Monster test_monster{"Death spawn"};
    test_monster.loadFromJSON("monsters-nodrops.json");

    std::cout << "Test stat is: " << test_monster.getInt("max_hit") << '\n';


    std::cout << "Current hp is " <<test_monster.getCurrentHP() << std::endl ; 

    test_monster.takeDMG(hit_value);

    std::cout << "HP after hitting for : " << hit_value << " is : " << test_monster.getCurrentHP();



    





    return 0;
}
*/

// using json = nlohmann::json;

// int main() {
//     std::ifstream i("monsters-complete.json");
//     json j;
//     i >> j;

//     // Iterate over all monsters and remove "drops"
//     for (auto& [key, monster] : j.items()) {
//         if (monster.contains("drops")) {
//             monster.erase("drops");
//         }
//     }

//     // Write back to a new file
//     std::ofstream o("monsters-nodrops.json");
//     o << std::setw(4) << j << std::endl;

//     return 0;
// }