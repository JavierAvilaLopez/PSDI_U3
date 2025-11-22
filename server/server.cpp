#include "clientManager.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    int port = defaultPort();
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        ::close(serverFd);
        return 1;
    }

    if (listen(serverFd, 8) < 0) {
        std::cerr << "Listen failed" << std::endl;
        ::close(serverFd);
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    std::map<uint32_t, std::unique_ptr<FileManager>> registry;
    std::mutex registryMutex;
    std::vector<std::unique_ptr<ClientManager>> clients;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (clientFd < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        auto manager = std::make_unique<ClientManager>(clientFd, registry, registryMutex);
        manager->start();
        clients.push_back(std::move(manager));
    }

    for (auto& c : clients) {
        c->join();
    }
    ::close(serverFd);
    return 0;
}
