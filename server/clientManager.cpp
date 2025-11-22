#include "clientManager.h"
#include "utils.h"
#include "../filemanager.h"

#include <unistd.h>
#include <atomic>
#include <iostream>

namespace {
std::atomic<uint32_t> globalNextId{1};
}

ClientManager::ClientManager(int socketFd, std::map<uint32_t, std::unique_ptr<FileManager>>& registry, std::mutex& registryMutex)
    : clientSocket(socketFd), fmRegistry(registry), registryMutex(registryMutex) {}

ClientManager::~ClientManager() {
    if (worker.joinable()) {
        worker.join();
    }
    if (clientSocket >= 0) {
        ::close(clientSocket);
        clientSocket = -1;
    }
}

void ClientManager::start() {
    worker = std::thread(&ClientManager::handleClient, this);
}

void ClientManager::join() {
    if (worker.joinable()) worker.join();
}

void ClientManager::handleClient() {
    while (true) {
        std::vector<uint8_t> request;
        if (!recvPacket(clientSocket, request)) {
            break;
        }
        std::vector<uint8_t> response;
        if (!processRequest(request, response)) {
            break;
        }
        if (!sendPacket(clientSocket, response)) {
            break;
        }
    }
    ::close(clientSocket);
    clientSocket = -1;
}

bool ClientManager::processRequest(const std::vector<uint8_t>& request, std::vector<uint8_t>& response) {
    size_t offset = 0;
    uint32_t method = readUint32(request, offset);
    switch (method) {
        case METHOD_CREATE: {
            uint32_t pathLen = readUint32(request, offset);
            std::string path;
            if (offset + pathLen <= request.size()) {
                path.assign(request.begin() + offset, request.begin() + offset + pathLen);
            }
            auto fm = std::make_unique<FileManager>(path);
            uint32_t id = globalNextId.fetch_add(1);
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                fmRegistry[id] = std::move(fm);
            }
            appendUint32(response, STATUS_OK);
            appendUint32(response, id);
            return true;
        }
        case METHOD_DESTROY: {
            uint32_t id = readUint32(request, offset);
            std::lock_guard<std::mutex> lock(registryMutex);
            auto it = fmRegistry.find(id);
            if (it != fmRegistry.end()) {
                fmRegistry.erase(it);
                appendUint32(response, STATUS_OK);
            } else {
                appendUint32(response, STATUS_ERROR);
            }
            appendUint32(response, id);
            return true;
        }
        case METHOD_LIST: {
            uint32_t id = readUint32(request, offset);
            std::unique_ptr<FileManager>* fmPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                auto it = fmRegistry.find(id);
                if (it != fmRegistry.end()) {
                    fmPtr = &it->second;
                }
            }
            if (!fmPtr) {
                appendUint32(response, STATUS_ERROR);
                appendUint32(response, 0);
                return true;
            }
            auto files = (*fmPtr)->listFiles();
            appendUint32(response, STATUS_OK);
            appendUint32(response, static_cast<uint32_t>(files.size()));
            for (const auto& f : files) {
                appendUint32(response, static_cast<uint32_t>(f.size()));
                response.insert(response.end(), f.begin(), f.end());
            }
            return true;
        }
        case METHOD_READ: {
            uint32_t id = readUint32(request, offset);
            uint32_t nameLen = readUint32(request, offset);
            std::string name;
            if (offset + nameLen <= request.size()) {
                name.assign(request.begin() + offset, request.begin() + offset + nameLen);
                offset += nameLen;
            }
            std::unique_ptr<FileManager>* fmPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                auto it = fmRegistry.find(id);
                if (it != fmRegistry.end()) fmPtr = &it->second;
            }
            if (!fmPtr) {
                appendUint32(response, STATUS_ERROR);
                appendUint32(response, 0);
                return true;
            }
            std::vector<unsigned char> data;
            (*fmPtr)->readFile(name, data);
            appendUint32(response, STATUS_OK);
            appendVector(response, data);
            return true;
        }
        case METHOD_WRITE: {
            uint32_t id = readUint32(request, offset);
            uint32_t nameLen = readUint32(request, offset);
            std::string name;
            if (offset + nameLen <= request.size()) {
                name.assign(request.begin() + offset, request.begin() + offset + nameLen);
                offset += nameLen;
            }
            std::vector<uint8_t> data;
            readVector(request, offset, data);

            std::unique_ptr<FileManager>* fmPtr = nullptr;
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                auto it = fmRegistry.find(id);
                if (it != fmRegistry.end()) fmPtr = &it->second;
            }
            if (!fmPtr) {
                appendUint32(response, STATUS_ERROR);
                appendUint32(response, 0);
                return true;
            }
            std::vector<unsigned char> writable(data.begin(), data.end());
            (*fmPtr)->writeFile(name, writable);
            appendUint32(response, STATUS_OK);
            appendUint32(response, static_cast<uint32_t>(writable.size()));
            return true;
        }
        default:
            return false;
    }
}
