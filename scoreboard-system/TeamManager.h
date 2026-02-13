#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

struct Player {
    std::string name;
    int number;
    std::string imagePath;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Player, name, number, imagePath)
};

struct Team {
    std::string name;
    std::vector<Player> players;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Team, name, players)
};

class TeamManager {
public:
    TeamManager(const std::string& dataDir);

    void loadTeams();
    void saveTeam(const std::string& teamName);
    
    void addOrUpdatePlayer(const std::string& teamName, const Player& player);
    void removePlayer(const std::string& teamName, int playerNumber);
    
    bool savePlayerImage(const std::string& teamName, int playerNumber, const std::vector<uint8_t>& imageData, const std::string& extension);
    std::vector<uint8_t> getPlayerImage(const std::string& teamName, int playerNumber) const;

    std::vector<std::string> getTeamNames() const;
    const Team* getTeam(const std::string& teamName) const;
    
    void deleteTeam(const std::string& teamName);

private:
    std::string dataDir;
    std::map<std::string, Team> teams;
    
    std::string getTeamFilePath(const std::string& teamName) const;
    std::string getImagesDirPath() const;
    void ensureDataDirectoryExists();
};
