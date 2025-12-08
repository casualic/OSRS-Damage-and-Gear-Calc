#include <iostream>
// #include <math>
#include "player.h"
#include <format>
// #include "httplib.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
// #include "cpp-httplib/httplib.h"
#include <curl/curl.h>
#include <map>
#include <sstream>
#include <vector>
#include <regex>


std::string skills[24] {
    "Overall", "Attack", "Defence", "Strength", "Hitpoints", "Ranged",
    "Prayer", "Magic", "Cooking", "Woodcutting", "Fletching", "Fishing",
    "Firemaking", "Crafting", "Smithing", "Mining", "Herblore", "Agility",
    "Thieving", "Slayer", "Farming", "Runecraft", "Hunter", "Construction"
};

// Correct
std::map<std::string, int> parseCSV(std::string csv_str) {

    std::map<std::string, int> res{{"test", 1}};

    std::stringstream ss(csv_str);
    std::string line;
    int i = 0;

    while (std::getline(ss, line, '\n') && i < 24) {
        std::stringstream line_ss(line);
        std::string skip, level;
        
        std::getline(line_ss, skip, ',');   // skip rank
        std::getline(line_ss, level, ',');  // get level
        
        res[skills[i]] = std::stoi(level);
        i++;
    };

    return res;
}


size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}


Player::Player(std::string n) : username(std::move(n)) {}
std::string Player::fetchStats() {
    CURL* curl = curl_easy_init();
    std::string response;
    std::string headers;
    
    if (curl) {
        std::string url = "https://secure.runescape.com/m=hiscore_oldschool/index_lite.ws?player=" + username;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Getting Header Data as well
        curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION, WriteCallback);
        curl_easy_setopt(curl,CURLOPT_HEADERDATA, &headers);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        // std::cout << response << std::endl;
    }
    return response;
}

void Player::parseStats(std::string raw_stats_response){
    stats_ = parseCSV(raw_stats_response);
}



