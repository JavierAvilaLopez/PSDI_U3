#include "filemanager.h"
#include "logging.h"
#include "rpc_utils.h"
#include <arpa/inet.h>
#include <atomic>
#include <filesystem>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <cstdlib>

struct ServerOptions {
    std::string host{"0.0.0.0"};
    int port{5001};
    std::string dir{"FileManagerDir"};
    std::string logFile;
    bool verbose{false};
};

static std::string env_or(const char *key, const std::string &fallback) {
    const char *v = std::getenv(key);
    return v ? std::string(v) : fallback;
}

ServerOptions parse_args(int argc, char **argv) {
    ServerOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--host" && i + 1 < argc) opts.host = argv[++i];
        else if (a == "--port" && i + 1 < argc) opts.port = std::atoi(argv[++i]);
        else if (a == "--dir" && i + 1 < argc) opts.dir = argv[++i];
        else if (a == "--log-file" && i + 1 < argc) opts.logFile = argv[++i];
        else if (a == "--verbose") opts.verbose = true;
    }
    return opts;
}

void respond_err(int fd, int connId, uint32_t code) {
    std::vector<uint8_t> payload;
    append_u32(payload, code);
    send_frame(fd, RpcOp::ERR, payload, "S", connId, "ERR");
}

void handle_client(int fd, int connId, const ServerOptions &opts) {
    LOG_INFO("S", connId, "accept", "new connection");
    bool ready = false;
    std::unique_ptr<FileManager> fm;
    RpcFrame frame;
    while (recv_frame(fd, frame, "S", connId)) {
        try {
            switch (frame.opcode) {
                case RpcOp::CTOR: {
                    size_t off = 0;
                    std::string requested = read_string(frame.payload, off);
                    std::string dir = opts.dir.empty() ? requested : opts.dir;

                    try {
                        if (!dir.empty()) {
                            std::filesystem::create_directories(dir);
                        }
                    } catch (const std::exception &e) {
                        LOG_WARN("S", connId, "CTOR", "cannot ensure dir exists " << e.what());
                    }

                    if (dir.empty()) {
                        respond_err(fd, connId, 4);
                        break;
                    }

                    fm = std::make_unique<FileManager>(dir);
                    ready = true;
                    std::vector<uint8_t> payload;
                    append_u32(payload, 0);
                    send_frame(fd, RpcOp::CTOR, payload, "S", connId, "CTOR");
                    break;
                }
                case RpcOp::LIST: {
                    if (!ready) { respond_err(fd, connId, 1); break; }
                    auto files = fm->listFiles();
                    std::vector<uint8_t> payload;
                    append_u32(payload, static_cast<uint32_t>(files.size()));
                    for (auto &f : files) append_string(payload, f);
                    send_frame(fd, RpcOp::LIST, payload, "S", connId, "LIST");
                    break;
                }
                case RpcOp::UPLOAD: {
                    if (!ready) { respond_err(fd, connId, 1); break; }
                    size_t off = 0;
                    std::string name = read_string(frame.payload, off);
                    uint32_t len = read_u32(frame.payload, off);
                    auto bytes = slice_payload(frame.payload, off, len);
                    std::vector<unsigned char> data(bytes.begin(), bytes.end());
                    fm->writeFile(name, data);
                    std::vector<uint8_t> payload;
                    append_u32(payload, 0);
                    send_frame(fd, RpcOp::UPLOAD, payload, "S", connId, "UPLOAD");
                    break;
                }
                case RpcOp::DOWNLOAD: {
                    if (!ready) { respond_err(fd, connId, 1); break; }
                    size_t off = 0;
                    std::string name = read_string(frame.payload, off);
                    std::vector<unsigned char> data;
                    fm->readFile(name, data);
                    std::vector<uint8_t> payload;
                    append_u32(payload, 0);
                    append_u32(payload, static_cast<uint32_t>(data.size()));
                    payload.insert(payload.end(), data.begin(), data.end());
                    send_frame(fd, RpcOp::DOWNLOAD, payload, "S", connId, "DOWNLOAD");
                    break;
                }
                case RpcOp::BYE: {
                    LOG_INFO("S", connId, "BYE", "client signaled bye");
                    close(fd);
                    return;
                }
                default:
                    respond_err(fd, connId, 2);
            }
        } catch (const std::exception &e) {
            LOG_ERR("S", connId, "handler", e.what());
            respond_err(fd, connId, 3);
        }
    }
    close(fd);
    LOG_INFO("S", connId, "disconnect", "connection closed");
}

int main(int argc, char **argv) {
    auto opts = parse_args(argc, argv);
    LogConfig cfg;
    cfg.role = "S";
    cfg.logFile = opts.logFile;
    cfg.level = opts.verbose ? LogLevel::DEBUG : level_from_string(env_or("FILEMGR_LOG", "info"));
    init_log(cfg);

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERR("S", -1, "socket", "cannot create socket");
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(opts.port);
    addr.sin_addr.s_addr = inet_addr(opts.host.c_str());
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("S", -1, "bind", "bind failed errno=" << errno);
        return 1;
    }
    listen(fd, 8);
    LOG_INFO("S", -1, "listen", "listening on " << opts.host << ':' << opts.port << " dir=" << opts.dir);

    std::atomic<int> counter{1};
    while (true) {
        int client_fd = accept(fd, nullptr, nullptr);
        if (client_fd < 0) {
            LOG_ERR("S", -1, "accept", "accept failed errno=" << errno);
            continue;
        }
        int connId = counter++;
        std::thread(handle_client, client_fd, connId, opts).detach();
    }
    return 0;
}

