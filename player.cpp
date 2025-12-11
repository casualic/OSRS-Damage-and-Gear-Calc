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
#include <fstream>
#include <thread>
#include <chrono>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "json.hpp"

using json = nlohmann::json;


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


void Player::fetchGearFromClient() {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace websocket = beast::websocket;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    net::io_context ioc;
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    // 1. Port Loop
    bool connected = false;
    for (int port = 37767; port <= 37776; ++port) {
        try {
            auto results = resolver.resolve("localhost", std::to_string(port));
            net::connect(ws.next_layer(), results);
            connected = true;
            std::cout << "Connected to WikiSync on port " << port << std::endl;
            break; 
        } catch (...) {
            continue; // Try next port
        }
    }

    if (!connected) {
        std::cerr << "Could not connect to WikiSync." << std::endl;
        return;
    }

    // 2. Handshake with Headers
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::origin, "https://tools.runescape.wiki");
            req.set(http::field::user_agent, "Mozilla/5.0 ...");
        }
    ));

    ws.handshake("localhost", "/");

    // 3. Send Discovery Requests
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

    // 4. Listen for Responses
    beast::flat_buffer buffer;
    std::cout << "Listening for responses..." << std::endl;
    
    int max_messages = 10;
    int message_count = 0;
    
    while(message_count < max_messages) {
        try {
            ws.read(buffer);
            std::string received = beast::buffers_to_string(buffer.data());
            
            // Check for player data
            if (received.find("\"_wsType\":\"GetPlayer\"") != std::string::npos || 
                received.find("\"_wsType\": \"GetPlayer\"") != std::string::npos) {
                
                std::cout << "✓ Found Player Data! Saving to file..." << std::endl;
                std::ofstream outfile("wikisync_data.json");
                outfile << received;
                outfile.close();
                std::cout << "Saved to wikisync_data.json" << std::endl;
                
                // Exit the loop after saving
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

void Player::loadGearStats(const std::string& itemDbPath) {
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

    // Traverse JSON safely
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

int Player::getEffectiveStat(const std::string& stat) {
    // This assumes 'stat' refers to a Base Skill (e.g., "Strength")
    // Use getEquipmentBonus for gear stats
    if (stats_.count(stat)) {
        return stats_.at(stat);
    }
    return 0;
}

int Player::getEquipmentBonus(const std::string& bonus) {
    int total = 0;
    for (auto& [slot, item] : gear_) {
        total += item.getInt(bonus);
    }
    return total;
}
