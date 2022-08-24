//
// Created by Marek Drozdik on 06/05/2022.
//

#ifndef SEMESTRAL_ROBOT_H
#define SEMESTRAL_ROBOT_H

#include <string>
#include <sstream>
#include "Direction.h"
#include "Coords.h"


#define NAME_SIZE 20
#define CLIENT_KEY_ID 5
#define SERVER_CLIENT_CONFIRMATION 7
#define CLIENT_OK 12
#define CLIENT_MESSAGE 100
constexpr int serverKeys[5] = {23019, 32037, 18789, 16443, 18189};
constexpr int clientKeys[5] = {32037, 29295, 13603, 29533, 21952};
constexpr Coords finish{0, 0};


int controlMessage(int socket, char *text, int maxBufferLen);

void sendMessage(const std::string &message, int socket);

using namespace std;
class Robot {
public:
    int socket;
    Coords loc{};
    Direction dir{};
    char name[NAME_SIZE]{};
    int nameLen{};

    explicit Robot(int s) : socket(s) {};

    void setRobotName();

    void exchangeHash();

    bool setPosition();

    bool navigate();

    void pickUpMessage() const;

private:
    int clientKeyId() const;

    int calculateServerHash();

    int calculateClientHash(int num, int keyID);

    void sendAndCheckHash(int keyID);

    void getLocationFromMessage(Coords &location) const;

    void setDirection(Coords loc1, Coords loc2);

    /*
     * returns true if finish was reached
     */
    bool updateLocation();

    bool getAroundObstacle();

    void discardMessage() const;

    bool avoidBarricade();

    void turnRobotUp();

    void turnRobotDown();

    void turnRobotRight();

    void turnRobotLeft();
};
#endif //SEMESTRAL_ROBOT_H
