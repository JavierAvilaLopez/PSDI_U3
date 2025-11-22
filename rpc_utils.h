#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include "logging.h"

constexpr uint32_t RPC_MAX_PAYLOAD = 16 * 1024 * 1024;

enum class RpcOp : uint8_t {
    HELLO = 1,
    CTOR = 2,
    LIST = 3,
    UPLOAD = 4,
    DOWNLOAD = 5,
    BYE = 6,
    ERR = 255
};

struct RpcFrame {
    RpcOp opcode{};
    std::vector<uint8_t> payload;
};

bool write_exact(int fd, const uint8_t *data, size_t len, const std::string &role, int connId, const std::string &op);
bool read_exact(int fd, uint8_t *data, size_t len, const std::string &role, int connId, const std::string &op);
bool send_frame(int fd, RpcOp op, const std::vector<uint8_t> &payload, const std::string &role, int connId, const std::string &opName);
bool recv_frame(int fd, RpcFrame &frame, const std::string &role, int connId);

void append_u32(std::vector<uint8_t> &buf, uint32_t v);
uint32_t read_u32(const std::vector<uint8_t> &buf, size_t &offset);
void append_string(std::vector<uint8_t> &buf, const std::string &s);
std::string read_string(const std::vector<uint8_t> &buf, size_t &offset);

std::vector<uint8_t> slice_payload(const std::vector<uint8_t> &buf, size_t offset, size_t len);

