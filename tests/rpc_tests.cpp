#include "rpc_utils.h"
#include <cassert>
#include <iostream>

int main() {
    // serialize/deserialize
    std::vector<uint8_t> buf;
    append_u32(buf, 42);
    append_string(buf, "demo");
    size_t off = 0;
    assert(read_u32(buf, off) == 42);
    assert(read_string(buf, off) == "demo");

    // frame assembly
    int fds[2];
    if (pipe(fds) != 0) return 1;
    send_frame(fds[1], RpcOp::LIST, buf, "T", 0, "TEST");
    RpcFrame f;
    if (!recv_frame(fds[0], f, "T", 0)) return 1;
    assert(f.opcode == RpcOp::LIST);
    size_t off2 = 0;
    assert(read_u32(f.payload, off2) == 42);
    assert(read_string(f.payload, off2) == "demo");
    std::cout << "rpc_tests ok" << std::endl;
    return 0;
}

