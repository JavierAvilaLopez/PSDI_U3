#include "../filemanager.h"
#include "clientManager.h"
#include "utils.h"

#include <unordered_map>
#include <mutex>

namespace {
struct RemoteState {
    ClientManager connection;
    uint32_t remoteId{0};
    bool active{false};
};

std::unordered_map<FileManager*, RemoteState> g_states;
std::mutex g_mutex;

bool sendCreate(RemoteState& state, const std::string& path) {
    std::vector<uint8_t> request;
    appendUint32(request, METHOD_CREATE);
    appendUint32(request, static_cast<uint32_t>(path.size()));
    request.insert(request.end(), path.begin(), path.end());
    if (!state.connection.sendPayload(request)) {
        return false;
    }
    std::vector<uint8_t> response;
    if (!state.connection.receivePayload(response)) {
        return false;
    }
    size_t offset = 0;
    uint32_t status = readUint32(response, offset);
    uint32_t id = readUint32(response, offset);
    if (status != STATUS_OK) {
        return false;
    }
    state.remoteId = id;
    state.active = true;
    return true;
}
}

FileManager::FileManager() {
    std::lock_guard<std::mutex> lock(g_mutex);
    RemoteState state;
    state.active = false;
    if (state.connection.connectTo(defaultHost(), defaultPort())) {
        sendCreate(state, "");
    }
    ready = state.active;
    g_states[this] = std::move(state);
}

FileManager::FileManager(std::string path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    RemoteState state;
    state.active = false;
    if (state.connection.connectTo(defaultHost(), defaultPort())) {
        sendCreate(state, path);
    }
    dirPath = path;
    ready = state.active;
    g_states[this] = std::move(state);
}

FileManager::~FileManager() {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_states.find(this);
    if (it == g_states.end()) return;
    RemoteState& state = it->second;
    if (state.active) {
        std::vector<uint8_t> request;
        appendUint32(request, METHOD_DESTROY);
        appendUint32(request, state.remoteId);
        state.connection.sendPayload(request);
        std::vector<uint8_t> response;
        state.connection.receivePayload(response);
    }
    state.connection.closeConnection();
    g_states.erase(it);
}

vector<string> FileManager::listFiles() {
    std::lock_guard<std::mutex> lock(g_mutex);
    vector<string> result;
    auto it = g_states.find(this);
    if (it == g_states.end()) return result;
    RemoteState& state = it->second;
    if (!state.active) return result;

    std::vector<uint8_t> request;
    appendUint32(request, METHOD_LIST);
    appendUint32(request, state.remoteId);
    if (!state.connection.sendPayload(request)) return result;

    std::vector<uint8_t> response;
    if (!state.connection.receivePayload(response)) return result;

    size_t offset = 0;
    uint32_t status = readUint32(response, offset);
    if (status != STATUS_OK) return result;
    uint32_t count = readUint32(response, offset);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t len = readUint32(response, offset);
        if (offset + len > response.size()) break;
        std::string name(response.begin() + offset, response.begin() + offset + len);
        offset += len;
        result.push_back(name);
    }
    return result;
}

void FileManager::readFile(string fileName, vector<unsigned char>& data) {
    std::lock_guard<std::mutex> lock(g_mutex);
    data.clear();
    auto it = g_states.find(this);
    if (it == g_states.end()) return;
    RemoteState& state = it->second;
    if (!state.active) return;

    std::vector<uint8_t> request;
    appendUint32(request, METHOD_READ);
    appendUint32(request, state.remoteId);
    appendUint32(request, static_cast<uint32_t>(fileName.size()));
    request.insert(request.end(), fileName.begin(), fileName.end());
    if (!state.connection.sendPayload(request)) return;

    std::vector<uint8_t> response;
    if (!state.connection.receivePayload(response)) return;

    size_t offset = 0;
    uint32_t status = readUint32(response, offset);
    if (status != STATUS_OK) return;
    readVector(response, offset, data);
}

void FileManager::writeFile(string fileName, vector<unsigned char>& data) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_states.find(this);
    if (it == g_states.end()) return;
    RemoteState& state = it->second;
    if (!state.active) return;

    std::vector<uint8_t> request;
    appendUint32(request, METHOD_WRITE);
    appendUint32(request, state.remoteId);
    appendUint32(request, static_cast<uint32_t>(fileName.size()));
    request.insert(request.end(), fileName.begin(), fileName.end());
    appendVector(request, data);
    if (!state.connection.sendPayload(request)) return;

    std::vector<uint8_t> response;
    state.connection.receivePayload(response);
}
