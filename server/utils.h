#pragma once

#include <cstdint>
#include <string>
#include <vector>

bool sendAll(int sock, const void* data, size_t len);
bool recvAll(int sock, void* data, size_t len);

bool sendPacket(int sock, const std::vector<uint8_t>& payload);
bool recvPacket(int sock, std::vector<uint8_t>& payload);

void appendUint32(std::vector<uint8_t>& buf, uint32_t value);
uint32_t readUint32(const std::vector<uint8_t>& buf, size_t& offset);
void appendVector(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data);
void readVector(const std::vector<uint8_t>& buf, size_t& offset, std::vector<uint8_t>& data);

constexpr uint32_t METHOD_CREATE = 1;
constexpr uint32_t METHOD_DESTROY = 2;
constexpr uint32_t METHOD_LIST = 3;
constexpr uint32_t METHOD_READ = 4;
constexpr uint32_t METHOD_WRITE = 5;

constexpr uint32_t STATUS_OK = 0;
constexpr uint32_t STATUS_ERROR = 1;

int defaultPort();
