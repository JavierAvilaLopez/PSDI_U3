#include "utils.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

bool sendAll(int sock, const void* data, size_t len) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sock, ptr + sent, len - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool recvAll(int sock, void* data, size_t len) {
    uint8_t* ptr = static_cast<uint8_t*>(data);
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = ::recv(sock, ptr + recvd, len - recvd, 0);
        if (n <= 0) {
            return false;
        }
        recvd += static_cast<size_t>(n);
    }
    return true;
}

bool sendPacket(int sock, const std::vector<uint8_t>& payload) {
    uint32_t size = htonl(static_cast<uint32_t>(payload.size()));
    if (!sendAll(sock, &size, sizeof(uint32_t))) {
        return false;
    }
    if (!payload.empty()) {
        return sendAll(sock, payload.data(), payload.size());
    }
    return true;
}

bool recvPacket(int sock, std::vector<uint8_t>& payload) {
    uint32_t sizeNetwork = 0;
    if (!recvAll(sock, &sizeNetwork, sizeof(uint32_t))) {
        return false;
    }
    uint32_t size = ntohl(sizeNetwork);
    payload.resize(size);
    if (size == 0) {
        return true;
    }
    return recvAll(sock, payload.data(), size);
}

bool sendString(int sock, const std::string& value) {
    uint32_t len = htonl(static_cast<uint32_t>(value.size()));
    if (!sendAll(sock, &len, sizeof(uint32_t))) {
        return false;
    }
    if (!value.empty()) {
        return sendAll(sock, value.data(), value.size());
    }
    return true;
}

bool recvString(int sock, std::string& value) {
    uint32_t lenNetwork = 0;
    if (!recvAll(sock, &lenNetwork, sizeof(uint32_t))) {
        return false;
    }
    uint32_t len = ntohl(lenNetwork);
    value.resize(len);
    if (len == 0) {
        return true;
    }
    return recvAll(sock, &value[0], len);
}

void appendUint32(std::vector<uint8_t>& buf, uint32_t value) {
    uint32_t net = htonl(value);
    auto ptr = reinterpret_cast<uint8_t*>(&net);
    buf.insert(buf.end(), ptr, ptr + sizeof(uint32_t));
}

uint32_t readUint32(const std::vector<uint8_t>& buf, size_t& offset) {
    uint32_t net = 0;
    if (offset + sizeof(uint32_t) > buf.size()) {
        return 0;
    }
    std::memcpy(&net, buf.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return ntohl(net);
}

void appendVector(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data) {
    appendUint32(buf, static_cast<uint32_t>(data.size()));
    if (!data.empty()) {
        buf.insert(buf.end(), data.begin(), data.end());
    }
}

void readVector(const std::vector<uint8_t>& buf, size_t& offset, std::vector<uint8_t>& data) {
    uint32_t len = readUint32(buf, offset);
    if (offset + len > buf.size()) {
        data.clear();
        offset = buf.size();
        return;
    }
    data.assign(buf.begin() + offset, buf.begin() + offset + len);
    offset += len;
}

std::string defaultHost() {
    const char* envHost = std::getenv("FM_HOST");
    if (envHost && std::strlen(envHost) > 0) {
        return std::string(envHost);
    }
    return "127.0.0.1";
}

int defaultPort() {
    const char* envPort = std::getenv("FM_PORT");
    if (envPort && std::strlen(envPort) > 0) {
        return std::atoi(envPort);
    }
    return 5555;
}
