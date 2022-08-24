//
// Created by Marek Drozdik on 06/05/2022.
//
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h> // socket(), bind(), connect(), listen()
#include <unistd.h> // close(), read(), write()
#include <netinet/in.h> // struct sockaddr_in
#include <strings.h> // bzero()
#include <sys/wait.h> // waitpid()
#include "Robot.h"


using namespace std;
#define BUFFER_SIZE 1000
#define TIMEOUT 1

int controlMessage(int socket, char *text, int maxBufferLen) {
    char buffer[BUFFER_SIZE];
    int curPos = 0;
    fd_set sockets;

    struct timeval timeout{};
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    int retval;
    while (true) {
        FD_ZERO(&sockets);
        FD_SET(socket, &sockets);
        retval = select(socket + 1, &sockets, nullptr, nullptr, &timeout);
        if (retval < 0) {
            close(socket);
            return -1;
        }
        if (!FD_ISSET(socket, &sockets)) {
            close(socket);
            return 0;
        }
        if (curPos > maxBufferLen - 2) {
            sendMessage("301 SYNTAX ERROR\a\b", socket);
            throw std::invalid_argument("Message is longer than maximal length.");
        }
        if (recv(socket, buffer, 1, 0) <= 0) {
            throw std::runtime_error("Failed to receive Message to robot.");
        }
        if (buffer[0] == '\a') {
            if (recv(socket, buffer, 1, 0) <= 0) {
                throw std::runtime_error("Failed to receive Message to robot.");
            }
            if (buffer[0] == '\b') {
                return curPos;
            } else {
                text[curPos++] = '\a';
            }
        }
        text[curPos++] = buffer[0];
    }
}

void sendMessage(const string &message, int socket) {
    if (send(socket, message.c_str(), message.length(), MSG_NOSIGNAL) == -1) {
        throw std::runtime_error("Failed to send Message to robot.");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: server port" << endl;
        return -1;
    }
    int socketL = socket(AF_INET, SOCK_STREAM, 0);
    if (socketL
        < 0) {
        perror("Nemohu vytvorit socket: ");
        return -1;
    }
    int port = atoi(argv[1]);
    if (port == 0) {
        cerr << "Usage: server port" << endl;
        close(socketL);
        return -1;
    }
    struct sockaddr_in adresa{};
    bzero(&adresa, sizeof(adresa));
    adresa.sin_family = AF_INET;
    adresa.sin_port = htons(port);
    adresa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(socketL, (struct sockaddr *) &adresa, sizeof(adresa)) < 0) {
        perror("Problem s bind(): ");
        close(socketL);
        return -1;
    }
    if (listen(socketL, 10) < 0) {
        perror("Problem s listen()!");
        close(socketL);
        return -1;
    }
    struct sockaddr_in vzdalena_adresa{};
    socklen_t velikost;
    while (true) {
        int cnctn = accept(socketL, (struct sockaddr *) &vzdalena_adresa, &velikost);
        if (cnctn < 0) {
            perror("Problem s accept()!");
            close(socketL);
            return -1;
        }
        pid_t pid = fork();
        if (pid == 0) {
            close(socketL);
            struct timeval timeout{};
            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;
            fd_set sockets;

            int retval;
            while (true) {
                FD_ZERO(&sockets);
                FD_SET(cnctn, &sockets);
                retval = select(cnctn + 1, &sockets, nullptr, nullptr, &timeout);
                if (retval < 0) {
                    perror("Chyba v select(): ");
                    close(cnctn);
                    return -1;
                }
                if (!FD_ISSET(cnctn, &sockets)) {
                    cout << "Connection timeout!" << endl;
                    close(cnctn);
                    return 0;
                }
                Robot rbt{cnctn};
                try {
                    rbt.setRobotName();
                    rbt.exchangeHash();
                    if (rbt.setPosition()) {
                        rbt.pickUpMessage();
                        close(cnctn);
                        return 0;
                    } else {
                        if (rbt.navigate()) {
                            rbt.pickUpMessage();
                            close(cnctn);
                            return 0;
                        }
                    }
                } catch (runtime_error) {
                    close(cnctn);
                    return -1;
                } catch (invalid_argument) {
                    close(cnctn);
                    return -1;
                }
            }
            return 0;
        }
        int status = 0;
        waitpid(0, &status, WNOHANG);
        close(cnctn);
    }
    close(socketL);
    return 0;
}


