// player.cpp
#include "player.h"
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>

#ifndef __EMSCRIPTEN__
#include <format>
#include <curl/curl.h>
#include <regex>
#include <thread>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#endif

#include "json.hpp"

using json = nlohmann::json;

std::string skills[24] {
    "Overall", "Attack", "Defence", "Strength", "Hitpoints", "Ranged",
    "Prayer", "Magic", "Cooking", "Woodcutting", "Fletching", "Fishing",
    "Firemaking", "Crafting", "Smithing", "Mining", "Herblore", "Agility",
    "Thieving", "Slayer", "Farming", "Runecraft", "Hunter", "Construction"
};

std::map<std::string, int> parseCSV(std::string csv_str) {
    std::map<std::string, int> res;
    std::stringstream ss(csv_str);
    std::string line;
    int i = 0;

    while (std::getline(ss, line, '\n') && i < 24) {
        std::stringstream line_ss(line);
        std::string skip, level;
        
        std::getline(line_ss, skip, ',');
        std::getline(line_ss, level, ',');
        
        try {
            res[skills[i]] = std::stoi(level);
        } catch (...) {
            res[skills[i]] = 1;
        }
        i++;
    }

    return res;
}

Player::Player(std::string n) : username(std::move(n)) {
    // Initialize default stats
    for (const auto& skill : skills) {
        stats_[skill] = 1;
    }
    currentHP_ = 99;
    maxHP_ = 99;
    piety_ = false;
    rigour_ = false;
    superCombat_ = false;
}

void Player::parseStats(std::string raw_stats_response) {
    auto parsed = parseCSV(raw_stats_response);
    for (const auto& [key, value] : parsed) {
        stats_[key] = value;
    }
    if (stats_.count("Hitpoints")) {
        maxHP_ = stats_["Hitpoints"];
        currentHP_ = maxHP_;
    }
}

void Player::equip(const std::string& slot, const Item& item) {
    gear_[slot] = item;
}

void Player::unequip(const std::string& slot) {
    gear_.erase(slot);
}

Item Player::getEquippedItem(const std::string& slot) const {
    auto it = gear_.find(slot);
    if (it != gear_.end()) {
        return it->second;
    }
    return Item();
}

bool Player::hasEquipped(const std::string& itemName) const {
    for (const auto& [slot, item] : gear_) {
        if (item.getName() == itemName) return true;
    }
    return false;
}

int Player::getEffectiveStat(const std::string& stat) {
    if (stats_.count(stat)) {
        return stats_.at(stat);
    }
    return 1;
}

int Player::getEquipmentBonus(const std::string& bonus) {
    int total = 0;
    for (auto& [slot, item] : gear_) {
        total += item.getInt(bonus);
    }
    return total;
}

int Player::getBoostedLevel(const std::string& skill) {
    int base = getEffectiveStat(skill);
    
    if (superCombat_) {
        if (skill == "Attack" || skill == "Strength" || skill == "Defence") {
            return base + 5 + static_cast<int>(base * 0.15);
        }
    }
    
    return base;
}

std::string Player::getActiveSet() {
    bool hasHead = gear_.count("head");
    bool hasBody = gear_.count("body");
    bool hasLegs = gear_.count("legs");
    bool hasHands = gear_.count("hands");
    
    if (!hasHead || !hasBody || !hasLegs) return "";
    
    std::string head = gear_.at("head").getName();
    std::string body = gear_.at("body").getName();
    std::string legs = gear_.at("legs").getName();
    std::string hands = hasHands ? gear_.at("hands").getName() : "";
    
    // Check Void
    if (hasHands && hands.find("Void knight gloves") != std::string::npos) {
        bool isEliteTop = body.find("Elite void top") != std::string::npos;
        bool isEliteLegs = legs.find("Elite void robe") != std::string::npos;
        bool isVoidTop = body.find("Void knight top") != std::string::npos;
        bool isVoidLegs = legs.find("Void knight robe") != std::string::npos;
        
        if ((isVoidTop || isEliteTop) && (isVoidLegs || isEliteLegs)) {
            bool isElite = isEliteTop && isEliteLegs;
            
            if (head.find("Void melee helm") != std::string::npos) {
                return isElite ? "Elite Void Melee" : "Void Melee";
            }
            if (head.find("Void ranger helm") != std::string::npos) {
                return isElite ? "Elite Void Range" : "Void Range";
            }
            if (head.find("Void mage helm") != std::string::npos) {
                return isElite ? "Elite Void Mage" : "Void Mage";
            }
        }
    }
    
    // Check Crystal
    if (head.find("Crystal helm") != std::string::npos &&
        body.find("Crystal body") != std::string::npos &&
        legs.find("Crystal legs") != std::string::npos) {
        return "Crystal";
    }
    
    // Check Inquisitor
    if (head.find("Inquisitor's great helm") != std::string::npos &&
        body.find("Inquisitor's hauberk") != std::string::npos &&
        legs.find("Inquisitor's plateskirt") != std::string::npos) {
        return "Inquisitor";
    }
    
    // Check Obsidian
    if (head.find("Obsidian helmet") != std::string::npos &&
        body.find("Obsidian platebody") != std::string::npos &&
        legs.find("Obsidian platelegs") != std::string::npos) {
        return "Obsidian";
    }
    
    // Check Dharok
    bool hasWeapon = gear_.count("weapon");
    if (hasWeapon) {
        std::string weapon = gear_.at("weapon").getName();
        if (head.find("Dharok's helm") != std::string::npos &&
            body.find("Dharok's platebody") != std::string::npos &&
            legs.find("Dharok's platelegs") != std::string::npos &&
            weapon.find("Dharok's greataxe") != std::string::npos) {
            return "Dharok";
        }
    }
    
    return "";
}

int Player::countCrystalPieces() {
    int count = 0;
    if (gear_.count("head") && gear_.at("head").getName().find("Crystal helm") != std::string::npos) count++;
    if (gear_.count("body") && gear_.at("body").getName().find("Crystal body") != std::string::npos) count++;
    if (gear_.count("legs") && gear_.at("legs").getName().find("Crystal legs") != std::string::npos) count++;
    return count;
}

#ifndef __EMSCRIPTEN__
// Native-only implementations

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string Player::fetchStats() {
    CURL* curl = curl_easy_init();
    std::string response;
    std::string headers;
    
    if (curl) {
        std::string url = "https://secure.runescape.com/m=hiscore_oldschool/index_lite.ws?player=" + username;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

void Player::fetchGearFromClient() {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace websocket = beast::websocket;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    bool connected = false;
    for (int port = 37767; port <= 37776; ++port) {
        try {
            auto results = resolver.resolve("localhost", std::to_string(port));
            net::connect(ws.next_layer(), results);
            connected = true;
            std::cout << "Connected to WikiSync on port " << port << std::endl;
            break; 
        } catch (...) {
            continue;
        }
    }

    if (!connected) {
        std::cerr << "Could not connect to WikiSync." << std::endl;
        return;
    }

    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::origin, "https://tools.runescape.wiki");
            req.set(http::field::user_agent, "Mozilla/5.0");
        }
    ));

    ws.handshake("localhost", "/");

    std::vector<std::string> requests = {
        "{\"type\":\"REQUEST_PLAYER_DATA\"}", 
        "{\"type\":\"GET_PLAYER\"}",
        "{\"type\":\"EQUIPMENT\"}",
        "{\"action\":\"get_equipment\"}",
        "{\"_wsType\":\"GetPlayer\"}"
    };

    for (const auto& req : requests) {
        std::cout << "Sending: " << req << std::endl;
        ws.write(net::buffer(req));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    beast::flat_buffer buffer;
    std::cout << "Listening for responses..." << std::endl;
    
    int max_messages = 10;
    int message_count = 0;
    
    while(message_count < max_messages) {
        try {
            ws.read(buffer);
            std::string received = beast::buffers_to_string(buffer.data());
            
            if (received.find("\"_wsType\":\"GetPlayer\"") != std::string::npos || 
                received.find("\"_wsType\": \"GetPlayer\"") != std::string::npos) {
                
                std::cout << "Found Player Data! Saving to file..." << std::endl;
                std::ofstream outfile("wikisync_data.json");
                outfile << received;
                outfile.close();
                std::cout << "Saved to wikisync_data.json" << std::endl;
                break;
            }
            
            buffer.consume(buffer.size()); 
            message_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error reading: " << e.what() << std::endl;
            break;
        }
    }
    
    try {
        ws.close(websocket::close_code::normal);
    } catch(...) {}
}

void Player::loadGearStats(const json& itemDb) {
    std::ifstream ifs("wikisync_data.json");
    if (!ifs.is_open()) {
        std::cerr << "Could not open wikisync_data.json. Run fetchGearFromClient first.\n";
        return;
    }
    
    json wsData;
    try {
        wsData = json::parse(ifs);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing wikisync_data.json: " << e.what() << "\n";
        return;
    }

    if (wsData.contains("payload") && 
        wsData["payload"].contains("loadouts") && 
        !wsData["payload"]["loadouts"].empty() &&
        wsData["payload"]["loadouts"][0].contains("equipment")) {
            
        auto equipment = wsData["payload"]["loadouts"][0]["equipment"];
        
        for (auto& [slot, data] : equipment.items()) {
            if (data.contains("id")) {
                int id = data["id"].get<int>();
                Item item(id);
                item.fetchStats(id, itemDb);
                gear_.emplace(slot, item);
                std::cout << "Loaded " << item.getName() << " (ID: " << id << ") into slot " << slot << "\n";
            }
        }
    } else {
        std::cerr << "Invalid wikisync_data.json structure.\n";
    }
}

void Player::loadGearStats(const std::string& itemDbPath) {
    std::ifstream dbIfs(itemDbPath);
    if (!dbIfs.is_open()) {
        std::cerr << "Could not open " << itemDbPath << "\n";
        return;
    }
    
    json itemDb;
    try {
        itemDb = json::parse(dbIfs);
    } catch(const std::exception& e) {
        std::cerr << "Error parsing item DB: " << e.what() << "\n";
        return;
    }
    
    loadGearStats(itemDb);
}

#endif // __EMSCRIPTEN__
