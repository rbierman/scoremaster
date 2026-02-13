#include "TeamManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

TeamManager::TeamManager(const std::string& dataDir) : dataDir(dataDir) {
    ensureDataDirectoryExists();
    loadTeams();
}

void TeamManager::ensureDataDirectoryExists() {
    if (!fs::exists(dataDir)) {
        try {
            fs::create_directories(dataDir);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating data directory: " << e.what() << std::endl;
        }
    }
    std::string imagesDir = getImagesDirPath();
    if (!fs::exists(imagesDir)) {
        try {
            fs::create_directories(imagesDir);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating images directory: " << e.what() << std::endl;
        }
    }
}

std::string TeamManager::getTeamFilePath(const std::string& teamName) const {
    return (fs::path(dataDir) / (teamName + ".json")).string();
}

std::string TeamManager::getImagesDirPath() const {
    return (fs::path(dataDir) / "images").string();
}

void TeamManager::loadTeams() {
    if (!fs::exists(dataDir)) return;

    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            try {
                std::ifstream file(entry.path());
                nlohmann::json j;
                file >> j;
                Team team = j.get<Team>();
                teams[team.name] = team;
            } catch (const std::exception& e) {
                std::cerr << "Error loading team from " << entry.path() << ": " << e.what() << std::endl;
            }
        }
    }
}

void TeamManager::saveTeam(const std::string& teamName) {
    auto it = teams.find(teamName);
    if (it == teams.end()) return;

    try {
        std::ofstream file(getTeamFilePath(teamName));
        nlohmann::json j = it->second;
        file << j.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "Error saving team " << teamName << ": " << e.what() << std::endl;
    }
}

void TeamManager::addOrUpdatePlayer(const std::string& teamName, const Player& player) {
    auto& team = teams[teamName];
    team.name = teamName;
    
    bool found = false;
    for (auto& p : team.players) {
        if (p.number == player.number) {
            p.name = player.name;
            found = true;
            break;
        }
    }
    
    if (!found) {
        team.players.push_back(player);
    }
    
    saveTeam(teamName);
}

void TeamManager::removePlayer(const std::string& teamName, int playerNumber) {
    auto it = teams.find(teamName);
    if (it != teams.end()) {
        auto& players = it->second.players;
        players.erase(std::remove_if(players.begin(), players.end(),
            [playerNumber](const Player& p) { return p.number == playerNumber; }),
            players.end());
        saveTeam(teamName);
    }
}

bool TeamManager::hasPlayer(const std::string& teamName, int playerNumber) const {
    auto it = teams.find(teamName);
    if (it == teams.end()) return false;
    for (const auto& p : it->second.players) {
        if (p.number == playerNumber) return true;
    }
    return false;
}

bool TeamManager::savePlayerImage(const std::string& teamName, int playerNumber, const std::vector<uint8_t>& imageData, const std::string& extension) {
    std::string fileName = teamName + "_" + std::to_string(playerNumber) + extension;
    fs::path imagePath = fs::path(getImagesDirPath()) / fileName;

    std::cout << "[TeamManager] Saving image to: " << imagePath << " (Size: " << imageData.size() << " bytes)" << std::endl;

    try {
        std::ofstream file(imagePath, std::ios::binary);
        if (!file) {
            std::cerr << "[TeamManager] Failed to open file for writing: " << imagePath << std::endl;
            return false;
        }
        file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        
        // Update player imagePath in memory
        auto& team = teams[teamName];
        for (auto& p : team.players) {
            if (p.number == playerNumber) {
                p.imagePath = fileName;
                saveTeam(teamName);
                return true;
            }
        }
        std::cerr << "[TeamManager] Player #" << playerNumber << " not found in team " << teamName << " after saving image." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[TeamManager] Error saving player image: " << e.what() << std::endl;
    }
    return false;
}

std::vector<uint8_t> TeamManager::getPlayerImage(const std::string& teamName, int playerNumber) const {
    auto it = teams.find(teamName);
    if (it == teams.end()) return {};

    std::string fileName = "";
    for (const auto& p : it->second.players) {
        if (p.number == playerNumber) {
            fileName = p.imagePath;
            break;
        }
    }

    if (fileName.empty()) return {};

    fs::path fullPath = fs::path(getImagesDirPath()) / fileName;
    if (!fs::exists(fullPath)) return {};

    try {
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading player image: " << e.what() << std::endl;
    }
    return {};
}

std::vector<std::string> TeamManager::getTeamNames() const {
    std::vector<std::string> names;
    for (const auto& [name, team] : teams) {
        names.push_back(name);
    }
    return names;
}

const Team* TeamManager::getTeam(const std::string& teamName) const {
    auto it = teams.find(teamName);
    if (it != teams.end()) {
        return &it->second;
    }
    return nullptr;
}

void TeamManager::deleteTeam(const std::string& teamName) {
    auto it = teams.find(teamName);
    if (it != teams.end()) {
        try {
            fs::remove(getTeamFilePath(teamName));
            teams.erase(it);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error deleting team file: " << e.what() << std::endl;
        }
    }
}
