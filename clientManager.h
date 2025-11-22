#pragma once
#include <string>
#include <vector>
#include "rpc_utils.h"

class RpcChannel {
public:
    RpcChannel(std::string host, int port, std::string role);
    ~RpcChannel();
    bool connect_now();
    void close_now();
    RpcFrame request(RpcOp op, const std::vector<uint8_t> &payload, const std::string &opName);
    bool is_ready() const { return fd_ >= 0; }
    int conn_id() const { return connId_; }
private:
    int fd_{-1};
    std::string host_;
    int port_{};
    std::string role_;
    int connId_{};
};

