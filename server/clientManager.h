#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

#include "../filemanager.h"

class ClientManager {
public:
    ClientManager(int socketFd, std::map<uint32_t, std::unique_ptr<FileManager>>& registry, std::mutex& registryMutex);
    ~ClientManager();

    void start();
    void join();

private:
    void handleClient();
    bool processRequest(const std::vector<uint8_t>& request, std::vector<uint8_t>& response);

    int clientSocket;
    std::thread worker;
    std::map<uint32_t, std::unique_ptr<FileManager>>& fmRegistry;
    std::mutex& registryMutex;
};
