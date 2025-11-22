#include "filemanager.h"
#include "clientManager.h"
#include "logging.h"
#include "rpc_utils.h"
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>

namespace {
std::string env_or(const char *key, const std::string &fallback) {
    const char *v = std::getenv(key);
    return v ? std::string(v) : fallback;
}

int env_or_int(const char *key, int fallback) {
    const char *v = std::getenv(key);
    if (!v) return fallback;
    return std::atoi(v);
}

std::map<FileManager*, std::unique_ptr<RpcChannel>> g_channels;
std::mutex g_ch_mutex;

RpcChannel* channel_for(FileManager *fm) {
    std::lock_guard<std::mutex> lk(g_ch_mutex);
    auto it = g_channels.find(fm);
    return it == g_channels.end() ? nullptr : it->second.get();
}

void set_channel(FileManager *fm, std::unique_ptr<RpcChannel> ch) {
    std::lock_guard<std::mutex> lk(g_ch_mutex);
    g_channels[fm] = std::move(ch);
}

void erase_channel(FileManager *fm) {
    std::lock_guard<std::mutex> lk(g_ch_mutex);
    g_channels.erase(fm);
}
}

FileManager::FileManager() : FileManager(env_or("FILEMGR_DIR", "FileManagerDir")) {}

FileManager::FileManager(std::string path) : dirPath(path), ready(false) {
    std::string host = env_or("FILEMGR_HOST", "127.0.0.1");
    int port = env_or_int("FILEMGR_PORT", 5001);
    LogConfig cfg;
    cfg.role = "C";
    std::string lvl = env_or("FILEMGR_LOG", "0");
    cfg.level = level_from_string(lvl);
    init_log(cfg);
    auto ch = std::make_unique<RpcChannel>(host, port, "C");
    vector<uint8_t> payload;
    append_string(payload, path);
    auto resp = ch->request(RpcOp::CTOR, payload, "CTOR");
    if (resp.opcode == RpcOp::CTOR && !resp.payload.empty() && resp.payload[0] == 0) {
        ready = true;
        set_channel(this, std::move(ch));
        LOG_DEBUG("C", channel_for(this)->conn_id(), "CTOR", "remote ctor ok dir=" << path);
    } else {
        LOG_ERR("C", ch->conn_id(), "CTOR", "ctor failed");
    }
}

FileManager::~FileManager() {
    if (auto ch = channel_for(this)) {
        ch->request(RpcOp::BYE, {}, "BYE");
        erase_channel(this);
    }
}

vector<string> FileManager::listFiles() {
    vector<string> files;
    auto ch = channel_for(this);
    if (!ready || !ch) return files;
    auto resp = ch->request(RpcOp::LIST, {}, "LIST");
    if (resp.opcode != RpcOp::LIST) {
        LOG_ERR("C", ch->conn_id(), "LIST", "unexpected opcode");
        return files;
    }
    size_t off = 0;
    try {
        uint32_t count = read_u32(resp.payload, off);
        for (uint32_t i = 0; i < count; ++i) {
            files.push_back(read_string(resp.payload, off));
        }
    } catch (const std::exception &e) {
        LOG_ERR("C", ch->conn_id(), "LIST", "decode error " << e.what());
    }
    return files;
}

void FileManager::readFile(string fileName, vector<unsigned char> &data) {
    data.clear();
    auto ch = channel_for(this);
    if (!ready || !ch) return;
    vector<uint8_t> payload;
    auto remoteName = path(fileName).filename().string();
    append_string(payload, remoteName);
    auto resp = ch->request(RpcOp::DOWNLOAD, payload, "DOWNLOAD");
    size_t off = 0;
    try {
        if (resp.opcode != RpcOp::DOWNLOAD) {
            LOG_ERR("C", ch->conn_id(), "DOWNLOAD", "unexpected opcode");
            return;
        }
        uint32_t status = read_u32(resp.payload, off);
        if (status != 0) {
            LOG_WARN("C", ch->conn_id(), "DOWNLOAD", "status=" << status);
            return;
        }
        uint32_t len = read_u32(resp.payload, off);
        auto bytes = slice_payload(resp.payload, off, len);
        data.assign(bytes.begin(), bytes.end());
        LOG_DEBUG("C", ch->conn_id(), "DOWNLOAD", "received bytes=" << data.size());
    } catch (const std::exception &e) {
        LOG_ERR("C", ch->conn_id(), "DOWNLOAD", "decode error " << e.what());
    }
}

void FileManager::writeFile(string fileName, vector<unsigned char> &data) {
    auto ch = channel_for(this);
    if (!ready || !ch) return;
    vector<uint8_t> payload;
    auto remoteName = path(fileName).filename().string();
    append_string(payload, remoteName);
    append_u32(payload, static_cast<uint32_t>(data.size()));
    payload.insert(payload.end(), data.begin(), data.end());
    auto resp = ch->request(RpcOp::UPLOAD, payload, "UPLOAD");
    size_t off = 0;
    try {
        if (resp.opcode != RpcOp::UPLOAD) {
            LOG_ERR("C", ch->conn_id(), "UPLOAD", "unexpected opcode");
            return;
        }
        uint32_t status = read_u32(resp.payload, off);
        LOG_DEBUG("C", ch->conn_id(), "UPLOAD", "status=" << status << " bytes=" << data.size());
    } catch (const std::exception &e) {
        LOG_ERR("C", ch->conn_id(), "UPLOAD", "decode error " << e.what());
    }
}

