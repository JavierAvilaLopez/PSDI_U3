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
    std::cout << "[client] Creating socket..." << std::endl; // DEBUG: added
    socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        std::perror("[client] socket"); // DEBUG: added
        return false;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr)); // FIX: ensure the whole sockaddr_in is zeroed
    std::cout << "[client] Zeroed sockaddr_in" << std::endl; // DEBUG: added

    addr.sin_family = AF_INET;
    std::cout << "[client] sin_family set to AF_INET" << std::endl; // DEBUG: added

    addr.sin_port = htons(static_cast<uint16_t>(port));
    std::cout << "[client] sin_port set (network order): " << addr.sin_port << std::endl; // DEBUG: added

    int ptonResult = ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    std::cout << "[client] inet_pton result: " << ptonResult << std::endl; // DEBUG: added
    std::cout << "[client] sin_addr (binary s_addr): " << addr.sin_addr.s_addr << std::endl; // DEBUG: added
    if (ptonResult <= 0) {
        std::perror("[client] inet_pton"); // DEBUG: added
        closeConnection();
        return false;
    }

    std::cout << "[client] Connecting to " << host << ":" << port << "..." << std::endl; // DEBUG: added
    int connectResult = ::connect(socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    std::cout << "[client] connect() returned " << connectResult << std::endl; // DEBUG: added
    if (connectResult < 0) {
        std::perror("connect"); // DEBUG: added
        closeConnection();
        return false;
    }
    std::cout << "[client] Connected" << std::endl; // DEBUG: added
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
