#include "rpc_utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <chrono>

bool write_exact(int fd, const uint8_t *data, size_t len, const std::string &role, int connId, const std::string &op) {
    size_t written = 0;
    while (written < len) {
        ssize_t r = ::write(fd, data + written, len - written);
        if (r < 0) {
            if (errno == EINTR) continue;
            LOG_ERR(role, connId, op, "write failed errno=" << errno);
            return false;
        }
        written += static_cast<size_t>(r);
        LOG_DEBUG(role, connId, op, "write chunk=" << r << " total=" << written << "/" << len);
    }
    return true;
}

bool read_exact(int fd, uint8_t *data, size_t len, const std::string &role, int connId, const std::string &op) {
    size_t readn = 0;
    while (readn < len) {
        ssize_t r = ::read(fd, data + readn, len - readn);
        if (r == 0) {
            LOG_WARN(role, connId, op, "peer closed while reading");
            return false;
        }
        if (r < 0) {
            if (errno == EINTR) continue;
            LOG_ERR(role, connId, op, "read failed errno=" << errno);
            return false;
        }
        readn += static_cast<size_t>(r);
        LOG_DEBUG(role, connId, op, "read chunk=" << r << " total=" << readn << "/" << len);
    }
    return true;
}

bool send_frame(int fd, RpcOp op, const std::vector<uint8_t> &payload, const std::string &role, int connId, const std::string &opName) {
    uint32_t length = static_cast<uint32_t>(payload.size() + 1);
    if (length > RPC_MAX_PAYLOAD) {
        LOG_ERR(role, connId, opName, "payload too large" << length);
        return false;
    }
    uint32_t netlen = htonl(length);
    std::vector<uint8_t> header(sizeof(netlen) + 1);
    memcpy(header.data(), &netlen, sizeof(netlen));
    header[4] = static_cast<uint8_t>(op);
    if (!write_exact(fd, header.data(), header.size(), role, connId, opName)) return false;
    if (!payload.empty() && !write_exact(fd, payload.data(), payload.size(), role, connId, opName)) return false;
    LOG_DEBUG(role, connId, opName, "sent frame opcode=" << static_cast<int>(op) << " payload=" << payload.size());
    return true;
}

bool recv_frame(int fd, RpcFrame &frame, const std::string &role, int connId) {
    uint8_t header[5];
    if (!read_exact(fd, header, sizeof(header), role, connId, "frame-header")) return false;
    uint32_t netlen;
    memcpy(&netlen, header, sizeof(netlen));
    uint32_t length = ntohl(netlen);
    if (length == 0 || length > RPC_MAX_PAYLOAD) {
        LOG_ERR(role, connId, "recv", "invalid frame length=" << length);
        return false;
    }
    frame.opcode = static_cast<RpcOp>(header[4]);
    std::vector<uint8_t> payload(length - 1);
    if (!payload.empty()) {
        if (!read_exact(fd, payload.data(), payload.size(), role, connId, "frame-payload")) return false;
    }
    frame.payload = std::move(payload);
    LOG_DEBUG(role, connId, "recv", "frame opcode=" << static_cast<int>(frame.opcode) << " payload=" << frame.payload.size());
    return true;
}

void append_u32(std::vector<uint8_t> &buf, uint32_t v) {
    uint32_t net = htonl(v);
    auto *p = reinterpret_cast<uint8_t *>(&net);
    buf.insert(buf.end(), p, p + sizeof(net));
}

uint32_t read_u32(const std::vector<uint8_t> &buf, size_t &offset) {
    if (offset + 4 > buf.size()) throw std::runtime_error("buffer underflow");
    uint32_t net;
    memcpy(&net, buf.data() + offset, 4);
    offset += 4;
    return ntohl(net);
}

void append_string(std::vector<uint8_t> &buf, const std::string &s) {
    append_u32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

std::string read_string(const std::vector<uint8_t> &buf, size_t &offset) {
    uint32_t len = read_u32(buf, offset);
    if (offset + len > buf.size()) throw std::runtime_error("buffer underflow");
    std::string s(reinterpret_cast<const char*>(buf.data() + offset), len);
    offset += len;
    return s;
}

std::vector<uint8_t> slice_payload(const std::vector<uint8_t> &buf, size_t offset, size_t len) {
    if (offset + len > buf.size()) throw std::runtime_error("buffer underflow");
    return std::vector<uint8_t>(buf.begin() + offset, buf.begin() + offset + len);
}

