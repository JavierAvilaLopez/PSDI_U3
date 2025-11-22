#include "clientManager.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>

namespace {
std::atomic<int> g_conn_counter{1};
}

RpcChannel::RpcChannel(std::string host, int port, std::string role)
    : host_(std::move(host)), port_(port), role_(std::move(role)), connId_(g_conn_counter++) {}

RpcChannel::~RpcChannel() { close_now(); }

bool RpcChannel::connect_now() {
    if (fd_ >= 0) return true;
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        LOG_ERR(role_, connId_, "connect", "cannot create socket");
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        LOG_ERR(role_, connId_, "connect", "invalid host " << host_);
        close_now();
        return false;
    }
    LOG_INFO(role_, connId_, "connect", "connecting to " << host_ << ':' << port_);
    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR(role_, connId_, "connect", "connect failed errno=" << errno);
        close_now();
        return false;
    }
    LOG_INFO(role_, connId_, "connect", "connected");
    return true;
}

void RpcChannel::close_now() {
    if (fd_ >= 0) {
        ::close(fd_);
        LOG_INFO(role_, connId_, "close", "connection closed");
    }
    fd_ = -1;
}

RpcFrame RpcChannel::request(RpcOp op, const std::vector<uint8_t> &payload, const std::string &opName) {
    RpcFrame response{};
    if (!connect_now()) return response;
    auto start = std::chrono::steady_clock::now();
    if (!send_frame(fd_, op, payload, role_, connId_, opName)) {
        close_now();
        return response;
    }
    if (!recv_frame(fd_, response, role_, connId_)) {
        close_now();
    }
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO(role_, connId_, opName, "roundtrip=" << ms << "ms");
    return response;
}

