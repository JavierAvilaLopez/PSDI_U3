#pragma once

#include <string>
#include <vector>
#include <cstdint>

class ClientManager {
public:
    ClientManager();
    explicit ClientManager(int sockFd);
    ~ClientManager();

    bool connectTo(const std::string& host, int port);
    bool isConnected() const { return socketFd >= 0; }
    void closeConnection();

    bool sendPayload(const std::vector<uint8_t>& data);
    bool receivePayload(std::vector<uint8_t>& data);

    int socket() const { return socketFd; }

private:
    int socketFd;
};
