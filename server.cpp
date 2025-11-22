#include "server/utils.h"
#include "server/clientManager.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>

int main(int argc, char** argv) {
    int port = defaultPort();
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "[server] Failed to create socket\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(serverSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[server] Failed to bind on port " << port << "\n";
        ::close(serverSocket);
        return 1;
    }

    if (::listen(serverSocket, 8) < 0) {
        std::cerr << "[server] Failed to listen on port " << port << "\n";
        ::close(serverSocket);
        return 1;
    }

    std::cout << "[server] Listening on port " << port << "..." << std::endl;

    std::map<uint32_t, std::unique_ptr<FileManager>> fmRegistry;
    std::mutex registryMutex;
    std::list<std::unique_ptr<ClientManager>> clients;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = ::accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            std::cerr << "[server] Error accepting connection\n";
            break;
        }

        auto manager = std::make_unique<ClientManager>(clientSocket, fmRegistry, registryMutex);
        manager->start();
        clients.push_back(std::move(manager));
    }

    for (auto& manager : clients) {
        manager->join();
    }

    ::close(serverSocket);
    return 0;
}
