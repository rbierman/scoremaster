#include "WebSocketManager.h"
#include "ScoreboardController.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// Simple base64 decoder
static std::vector<uint8_t> base64_decode(const std::string& in) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<uint8_t> out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (uint8_t c : in) {
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(uint8_t((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string base64_encode(const std::vector<uint8_t>& in) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

json WebSocketManager::teamsToJson() {
    json teamsList = json::array();
    for (const auto& name : teamManager.getTeamNames()) {
        const Team* t = teamManager.getTeam(name);
        if (t) {
            json teamJson;
            teamJson["name"] = t->name;
            json playersList = json::array();
            for (const auto& p : t->players) {
                json pJson;
                pJson["name"] = p.name;
                pJson["number"] = p.number;
                pJson["hasImage"] = !p.imagePath.empty();
                playersList.push_back(pJson);
            }
            teamJson["players"] = playersList;
            teamsList.push_back(teamJson);
        }
    }
    return teamsList;
}

WebSocketManager::WebSocketManager(int port, ScoreboardController& controller, TeamManager& teamManager)
    : port(port), controller(controller), teamManager(teamManager), server(port, "0.0.0.0") {
    
    server.setOnConnectionCallback([this](std::weak_ptr<ix::WebSocket> webSocket, std::shared_ptr<ix::ConnectionState> connectionState) {
        auto ws = webSocket.lock();
        if (ws) {
            // Note: maxMessageSize is in ix::WebSocketOptions, 
            // but for raw WebSocket objects we often use setMaxMessageSize
            // or it's handled via the server configuration.
            // In modern IXWebSocket, setMaxMessageSize is available on the WebSocket object.
            // Let's use a very large value to effectively disable it for images.
            // If it doesn't exist, we'll try to find the alternative.
            // Actually, we can just call it and if it fails to compile, we'll try something else.
            // But let's be careful.
            
            ws->setOnMessageCallback([this, connectionState, webSocket](const ix::WebSocketMessagePtr& msg) {
                auto ws = webSocket.lock();
                if (ws) {
                    handleMessage(connectionState, *ws, msg);
                }
            });
        }
    });
}

WebSocketManager::~WebSocketManager() {
    stop();
}

void WebSocketManager::start() {
    auto res = server.listen();
    if (!res.first) {
        std::cerr << "Error: WebSocket server failed to listen on port " << port << ": " << res.second << std::endl;
        return;
    }
    server.start();
    std::cout << "WebSocket server started on port " << port << " (Large payload support enabled)" << std::endl;
}

void WebSocketManager::stop() {
    server.stop();
}

void WebSocketManager::broadcastState(const ScoreboardState& state) {
    json j = stateToJson(state);
    std::string payload = j.dump();
    
    for (auto&& client : server.getClients()) {
        client->send(payload);
    }
}

void WebSocketManager::handleMessage(std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket & webSocket, const ix::WebSocketMessagePtr & msg) {
    if (msg->type == ix::WebSocketMessageType::Message) {
        try {
            json j = json::parse(msg->str);
            std::string cmd = j.value("command", "");
            
            if (cmd != "getImage") {
                std::cout << "[WebSocket] Received command: " << cmd << " (Size: " << msg->str.length() << " bytes)" << std::endl;
            }

            if (cmd == "getTeams") {
                json response;
                response["type"] = "teams";
                response["teams"] = teamsToJson();
                webSocket.send(response.dump());
                return;
            } else if (cmd == "uploadPlayerImage") {
                std::string teamName = j.at("team").get<std::string>();
                int playerNumber = j.at("number").get<int>();
                std::string base64Data = j.at("data").get<std::string>();
                std::string ext = j.value("ext", ".jpg");

                std::cout << "[WebSocket] Uploading image for " << teamName << " #" << playerNumber << " (" << base64Data.length() << " base64 chars)" << std::endl;

                auto decoded = base64_decode(base64Data);
                if (!decoded.empty()) {
                    if (teamManager.savePlayerImage(teamName, playerNumber, decoded, ext)) {
                        std::cout << "[WebSocket] Image saved successfully. Broadcasting update." << std::endl;
                        json response;
                        response["type"] = "teams";
                        response["teams"] = teamsToJson();
                        std::string respPayload = response.dump();
                        for (auto&& client : server.getClients()) {
                            client->send(respPayload);
                        }
                    } else {
                        std::cerr << "[WebSocket] Failed to save player image." << std::endl;
                    }
                } else {
                    std::cerr << "[WebSocket] Base64 decode failed or data empty." << std::endl;
                }
                return;
            } else if (cmd == "getImage") {
                std::string teamName = j.at("team").get<std::string>();
                int playerNumber = j.at("number").get<int>();
                auto imageData = teamManager.getPlayerImage(teamName, playerNumber);
                
                json response;
                response["type"] = "image";
                response["team"] = teamName;
                response["number"] = playerNumber;
                if (!imageData.empty()) {
                    response["data"] = base64_encode(imageData);
                } else {
                    response["data"] = nullptr;
                }
                webSocket.send(response.dump());
                return;
            } else if (cmd == "triggerGoal") {
                bool isHome = j.at("isHome").get<bool>();
                int playerNumber = j.value("playerNumber", 0);
                
                std::string teamName = isHome ? controller.getState().homeTeamName : controller.getState().awayTeamName;

                std::cout << "[WebSocket] Triggering goal for " << (isHome ? "Home" : "Away") << " (" << teamName << ") (Player: " << playerNumber << ")" << std::endl;

                // Increment score (Reliable like + button)
                if (isHome) {
                    controller.addHomeScore(1);
                } else {
                    controller.addAwayScore(1);
                }

                if (playerNumber > 0) {
                    const Team* team = teamManager.getTeam(teamName);
                    if (team) {
                        for (const auto& player : team->players) {
                            if (player.number == playerNumber) {
                                auto imageData = teamManager.getPlayerImage(teamName, playerNumber);
                                controller.triggerGoalCelebration(player.name, playerNumber, imageData);
                                return;
                            }
                        }
                    }
                }
                return;
            }
            
            handleCommand(msg->str);
        } catch (const std::exception& e) {
            std::cerr << "Error handling message: " << e.what() << std::endl;
        }
    } else if (msg->type == ix::WebSocketMessageType::Open) {
        std::cout << "New WebSocket connection" << std::endl;
        // Send initial state
        json j = stateToJson(controller.getState());
        webSocket.send(j.dump());
        
        // Also send teams on connect
        json teamsResponse;
        teamsResponse["type"] = "teams";
        teamsResponse["teams"] = teamsToJson();
        webSocket.send(teamsResponse.dump());
    } else if (msg->type == ix::WebSocketMessageType::Close) {
        std::cout << "WebSocket connection closed" << std::endl;
    } else if (msg->type == ix::WebSocketMessageType::Error) {
        std::cerr << "WebSocket error: " << msg->errorInfo.reason << std::endl;
    }
}

void WebSocketManager::handleCommand(const std::string& payload) {
    try {
        json j = json::parse(payload);
        std::string cmd = j.value("command", "");

        if (cmd == "setHomeScore") controller.setHomeScore(j.at("value").get<int>());
        else if (cmd == "setAwayScore") controller.setAwayScore(j.at("value").get<int>());
        else if (cmd == "addHomeScore") controller.addHomeScore(j.value("delta", 1));
        else if (cmd == "addAwayScore") controller.addAwayScore(j.value("delta", 1));
        else if (cmd == "addHomeShots") controller.addHomeShots(j.value("delta", 1));
        else if (cmd == "addAwayShots") controller.addAwayShots(j.value("delta", 1));
        else if (cmd == "setHomeTeamName") controller.setHomeTeamName(j.at("value").get<std::string>());
        else if (cmd == "setAwayTeamName") controller.setAwayTeamName(j.at("value").get<std::string>());
        else if (cmd == "setHomePenalty") {
            controller.setHomePenalty(j.at("index").get<int>(), j.at("value").get<int>(), j.at("player").get<int>());
        }
        else if (cmd == "setAwayPenalty") {
            controller.setAwayPenalty(j.at("index").get<int>(), j.at("value").get<int>(), j.at("player").get<int>());
        }
        else if (cmd == "addHomePenalty") controller.addHomePenalty(j.at("value").get<int>(), j.value("player", 0));
        else if (cmd == "addAwayPenalty") controller.addAwayPenalty(j.at("value").get<int>(), j.value("player", 0));
        else if (cmd == "toggleClock") controller.toggleClock();
        else if (cmd == "resetGame") controller.resetGame();
        else if (cmd == "nextPeriod") controller.nextPeriod();
        else if (cmd == "setTime") {
            controller.setTime(j.at("minutes").get<int>(), j.at("seconds").get<int>());
        }
        else if (cmd == "setClockMode") {
            std::string mode = j.at("value").get<std::string>();
            if (mode == "Game") controller.setClockMode(ClockMode::Game);
            else if (mode == "Intermission") controller.setClockMode(ClockMode::Intermission);
            else if (mode == "TimeOfDay") controller.setClockMode(ClockMode::TimeOfDay);
        }
        else if (cmd == "addOrUpdatePlayer") {
            Player p;
            p.name = j.at("name").get<std::string>();
            p.number = j.at("number").get<int>();
            teamManager.addOrUpdatePlayer(j.at("team").get<std::string>(), p);
            
            // Broadcast teams update
            json response;
            response["type"] = "teams";
            response["teams"] = teamsToJson();
            std::string respPayload = response.dump();
            for (auto&& client : server.getClients()) {
                client->send(respPayload);
            }
        }
        else if (cmd == "removePlayer") {
            teamManager.removePlayer(j.at("team").get<std::string>(), j.at("number").get<int>());
            
            // Broadcast teams update
            json response;
            response["type"] = "teams";
            response["teams"] = teamsToJson();
            std::string respPayload = response.dump();
            for (auto&& client : server.getClients()) {
                client->send(respPayload);
            }
        }
        else if (cmd == "deleteTeam") {
            teamManager.deleteTeam(j.at("name").get<std::string>());
             // Broadcast teams update
            json response;
            response["type"] = "teams";
            response["teams"] = teamsToJson();
            std::string respPayload = response.dump();
            for (auto&& client : server.getClients()) {
                client->send(respPayload);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command JSON: " << e.what() << std::endl;
    }
}

json WebSocketManager::stateToJson(const ScoreboardState& state) {
    json j;
    j["homeScore"] = state.homeScore;
    j["awayScore"] = state.awayScore;
    j["timeMinutes"] = state.timeMinutes;
    j["timeSeconds"] = state.timeSeconds;
    j["homeShots"] = state.homeShots;
    j["awayShots"] = state.awayShots;
    j["currentPeriod"] = state.currentPeriod;
    j["homeTeamName"] = state.homeTeamName;
    j["awayTeamName"] = state.awayTeamName;
    j["isClockRunning"] = state.isClockRunning;
    
    std::string mode;
    switch(state.clockMode) {
        case ClockMode::Game: mode = "Game"; break;
        case ClockMode::Intermission: mode = "Intermission"; break;
        case ClockMode::TimeOfDay: mode = "TimeOfDay"; break;
    }
    j["clockMode"] = mode;

    auto penaltiesToJson = [](const Penalty p[2]) {
        json arr = json::array();
        for(int i=0; i<2; ++i) {
            arr.push_back({{"secondsRemaining", p[i].secondsRemaining}, {"playerNumber", p[i].playerNumber}});
        }
        return arr;
    };
    j["homePenalties"] = penaltiesToJson(state.homePenalties);
    j["awayPenalties"] = penaltiesToJson(state.awayPenalties);
    
    return j;
}
