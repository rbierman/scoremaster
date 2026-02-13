#include "WebSocketManager.h"
#include "ScoreboardController.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

WebSocketManager::WebSocketManager(int port, ScoreboardController& controller)
    : port(port), controller(controller), server(port, "0.0.0.0") {
    
    server.setOnConnectionCallback([this](std::weak_ptr<ix::WebSocket> webSocket, std::shared_ptr<ix::ConnectionState> connectionState) {
        auto ws = webSocket.lock();
        if (ws) {
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
    std::cout << "WebSocket server started on port " << port << std::endl;
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
        handleCommand(msg->str);
    } else if (msg->type == ix::WebSocketMessageType::Open) {
        std::cout << "New WebSocket connection" << std::endl;
        // Send initial state
        json j = stateToJson(controller.getState());
        webSocket.send(j.dump());
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
        else if (cmd == "toggleClock") controller.toggleClock();
        else if (cmd == "resetGame") controller.resetGame();
        else if (cmd == "nextPeriod") controller.nextPeriod();
        else if (cmd == "setClockMode") {
            std::string mode = j.at("value").get<std::string>();
            if (mode == "Running") controller.setClockMode(ClockMode::Running);
            else if (mode == "Stopped") controller.setClockMode(ClockMode::Stopped);
            else if (mode == "Clock") controller.setClockMode(ClockMode::Clock);
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
    
    std::string mode;
    switch(state.clockMode) {
        case ClockMode::Running: mode = "Running"; break;
        case ClockMode::Stopped: mode = "Stopped"; break;
        case ClockMode::Clock: mode = "Clock"; break;
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
