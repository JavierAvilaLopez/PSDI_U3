#include "clientManager.h"
#include "utils.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

ClientManager::ClientManager() : socketFd(-1) {}

ClientManager::ClientManager(int sockFd) : socketFd(sockFd) {}

ClientManager::~ClientManager() {
    closeConnection();
}

bool ClientManager::connectTo(const std::string& host, int port) {
    closeConnection();
    socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        closeConnection();
        return false;
    }

    if (::connect(socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        closeConnection();
        return false;
    }
    return true;
}

void ClientManager::closeConnection() {
    if (socketFd >= 0) {
        ::close(socketFd);
        socketFd = -1;
    }
}

bool ClientManager::sendPayload(const std::vector<uint8_t>& data) {
    if (socketFd < 0) {
        return false;
    }
    return sendPacket(socketFd, data);
}

bool ClientManager::receivePayload(std::vector<uint8_t>& data) {
    if (socketFd < 0) {
        return false;
    }
    return recvPacket(socketFd, data);
}
