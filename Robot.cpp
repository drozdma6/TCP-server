//
// Created by Marek Drozdik on 24/08/2022.
//

#include "Robot.h"
#include <iostream>
#include <unistd.h>


void Robot::setRobotName() {
    nameLen = controlMessage(socket, name, NAME_SIZE);
}


void Robot::exchangeHash() {
    sendMessage("107 KEY REQUEST\a\b", socket);
    int keyID = clientKeyId();
    sendAndCheckHash(keyID);
}

bool Robot::setPosition() {
    Coords first{};
    sendMessage("102 MOVE\a\b", socket);
    getLocationFromMessage(first);
    if (first == finish) {
        return true;
    }
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) {
        return true;
    }
    if (first == loc) {
        sendMessage("104 TURN RIGHT\a\b", socket);
        char buff[CLIENT_OK];
        controlMessage(socket, buff, CLIENT_OK);
        sendMessage("102 MOVE\a\b", socket);
        getLocationFromMessage(loc);
    }
    setDirection(first, loc);
    return false;
}

bool Robot::navigate() {
    Coords last{};
    last = loc;
    while (true) {
        loc.y > 0 ? turnRobotDown() : turnRobotUp();
        last = loc;
        sendMessage("102 MOVE\a\b", socket);
        if (updateLocation()) {
            return true;
        }
        if (loc.y == 0) {
            break;
        }
        if (last == loc) {
            if (avoidBarricade()) {
                return true;
            }
        }
    }
    while (loc.x != 0) {
        loc.x > 0 ? turnRobotLeft() : turnRobotRight();
        last = loc;
        sendMessage("102 MOVE\a\b", socket);
        if (updateLocation()) {
            return true;
        }
        if (last == loc) {
            if (getAroundObstacle()) {
                return true;
            }
        }
    }
    return false;
}

void Robot::pickUpMessage() const {
    sendMessage("105 GET MESSAGE\a\b", socket);
    char buff[CLIENT_MESSAGE];
    controlMessage(socket, buff, CLIENT_MESSAGE);
    sendMessage("106 LOGOUT\a\b", socket);
}

int Robot::clientKeyId() const {
    char bufferID[CLIENT_KEY_ID];
    int idLen = controlMessage(socket, bufferID, CLIENT_KEY_ID);
    for (size_t i = 0; i < idLen; ++i) {
        if (!isdigit(bufferID[i])) {
            cerr << "Wrong format of clientKeyId." << endl;
            sendMessage("301 SYNTAX ERROR\a\b", socket);
            throw std::invalid_argument("Invalid argument");
        }
    }
    int keyId = atoi(bufferID);
    if (idLen > 3 || keyId > 4 || keyId < 0) {
        sendMessage("303 KEY OUT OF RANGE\a\b", socket);
        throw std::invalid_argument("Key out of range.");
    }
    return keyId;
}

int Robot::calculateServerHash() {
    int hash = 0;
    for (size_t i = 0; i < nameLen; ++i) {
        hash += (int) name[i];
    }
    return (hash * 1000) % 65536;
}

int Robot::calculateClientHash(int num, int keyID) {
    int clientHash = (num - clientKeys[keyID]) % 65536;
    if (clientHash < 0) {
        return 65536 + clientHash;
    }
    return clientHash;
}

void Robot::sendAndCheckHash(int keyID) {
    int hash = calculateServerHash();

    string hashToSend = to_string(((hash + serverKeys[keyID]) % 65536));
    hashToSend += "\a\b";
    if (hashToSend.size() > SERVER_CLIENT_CONFIRMATION) {
        perror("Hash can have max 5 digits plus /a/b: ");
        throw std::runtime_error("Hash can have max 5 digits plus /a/b. ");
    }
    sendMessage(hashToSend, socket);
    char clientHashBuffer[SERVER_CLIENT_CONFIRMATION];
    int len = controlMessage(socket, clientHashBuffer, SERVER_CLIENT_CONFIRMATION);
    for (int i = 0; i < len; i++) {
        if (!isdigit(clientHashBuffer[i])) {
            sendMessage("301 SYNTAX ERROR\a\b", socket);
            throw std::invalid_argument("Hash needs to be numbers only.");
        }
    }
    if (calculateClientHash(atoi(clientHashBuffer), keyID) == hash) {
        sendMessage("200 OK\a\b", socket);
    } else {
        sendMessage("300 LOGIN FAILED\a\b", socket);
        close(socket);
    }
}

void Robot::getLocationFromMessage(Coords &location) const {
    char buff[CLIENT_OK];
    int len = controlMessage(socket, buff, CLIENT_OK);
    string strCoords;
    for (int i = 2; i < len; ++i) {
        if (isdigit(buff[i]) || buff[i] == ' ' || buff[i] == '-') {
            strCoords += buff[i];
        } else {
            sendMessage("301 SYNTAX ERROR\a\b", socket);
            throw std::invalid_argument("Floating point in coords.");
        }
    }
    stringstream s(strCoords);
    s >> location.x >> location.y;
    if (!s.eof()) {
        sendMessage("301 SYNTAX ERROR\a\b", socket);
        throw std::invalid_argument("Floating point in coords.");
    }
}

void Robot::setDirection(Coords loc1, Coords loc2) {
    if (loc2.x > loc1.x) { dir = Direction::RIGHT; }
    else if (loc2.x < loc1.x) { dir = Direction::LEFT; }
    else if (loc2.y > loc1.y) { dir = Direction::UP; }
    else if (loc2.y < loc1.y) { dir = Direction::DOWN; }
    else {
        throw std::runtime_error("Set direction was called with same coords.");
    }
}

bool Robot::updateLocation() {
    Coords tmp{};
    getLocationFromMessage(tmp);
    loc.x = tmp.x;
    loc.y = tmp.y;
    if (loc == finish) {
        return true;
    }
    return false;
}

bool Robot::getAroundObstacle() {
    sendMessage("104 TURN RIGHT\a\b", socket);
    discardMessage();
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) { return true; }
    sendMessage("103 TURN LEFT\a\b", socket);
    discardMessage();
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) { return true; }
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) { return true; }
    sendMessage("103 TURN LEFT\a\b", socket);
    discardMessage();
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) { return true; }
    sendMessage("104 TURN RIGHT\a\b", socket);
    discardMessage();
    return false;
}

void Robot::discardMessage() const {
    char buff[CLIENT_OK];
    controlMessage(socket, buff, CLIENT_OK);
}

bool Robot::avoidBarricade() {
    Coords rightCoord = loc + static_cast<Direction>((static_cast<size_t>(dir) + 1) % DIRECTIONS);
    Coords leftCoord = loc + static_cast<Direction>((static_cast<size_t>(dir) - 1) % DIRECTIONS);
    if (rightCoord.coordsWeight() < leftCoord.coordsWeight()) {
        sendMessage("104 TURN RIGHT\a\b", socket);
        dir = static_cast<Direction>((static_cast<size_t>(dir) + 1) % DIRECTIONS);
    } else {
        sendMessage("103 TURN LEFT\a\b", socket);
        dir = static_cast<Direction>((static_cast<size_t>(dir) - 1) % DIRECTIONS);
    }
    discardMessage();
    sendMessage("102 MOVE\a\b", socket);
    if (updateLocation()) {
        return true;
    }
    return false;
}

void Robot::turnRobotUp() {
    switch (dir) {
        case Direction::UP:
            return;
        case Direction::RIGHT:
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
        case Direction::LEFT:
            sendMessage("104 TURN RIGHT\a\b", socket);
            break;
        case Direction::DOWN:
            sendMessage("103 TURN LEFT\a\b", socket);
            discardMessage();
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
    }
    discardMessage();
    dir = Direction::UP;
}

void Robot::turnRobotDown() {
    switch (dir) {
        case Direction::DOWN:
            return;
        case Direction::RIGHT:
            sendMessage("104 TURN RIGHT\a\b", socket);
            break;
        case Direction::LEFT:
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
        case Direction::UP:
            sendMessage("103 TURN LEFT\a\b", socket);
            discardMessage();
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
    }
    discardMessage();
    dir = Direction::DOWN;
}

void Robot::turnRobotRight() {
    switch (dir) {
        case Direction::RIGHT:
            return;
        case Direction::UP:
            sendMessage("104 TURN RIGHT\a\b", socket);
            break;
        case Direction::DOWN:
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
        case Direction::LEFT:
            sendMessage("103 TURN LEFT\a\b", socket);
            discardMessage();
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
    }
    discardMessage();
    dir = Direction::RIGHT;
}

void Robot::turnRobotLeft() {
    switch (dir) {
        case Direction::LEFT:
            return;
        case Direction::UP:
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
        case Direction::DOWN:
            sendMessage("104 TURN RIGHT\a\b", socket);
            break;
        case Direction::RIGHT:
            sendMessage("103 TURN LEFT\a\b", socket);
            discardMessage();
            sendMessage("103 TURN LEFT\a\b", socket);
            break;
    }
    discardMessage();
    dir = Direction::LEFT;
}