# Distributed FileManager RPC Review

## Files Provided by the University
- `main_fm.cpp` (not modifiable)
- `fileManager.h` (not modifiable)
- `libFileManager.a` (not modifiable)
- Base CMake project

## Detected protocol and logic errors
1. **Client never initializes the remote session when server host/port differ from defaults.** The remote `FileManager` constructor always connects to `defaultHost()/defaultPort()` (environment or 127.0.0.1:5555) and immediately sends `METHOD_CREATE`. If that TCP connection fails (e.g., because the user runs `./client <ip> <port>` expecting those arguments to be used), `state.active` remains `false`. Subsequent `listFiles`, `writeFile`, and `readFile` return early without sending any RPC, so the server never sees uploads or listings. 【F:client/fileManagerRemoto.cpp†L42-L152】【F:client/utils.cpp†L116-L130】
2. **Command-line arguments are ignored by the client binary.** The only compiled `main` for the client is `main_fm.cpp`, which neither parses `argv` nor calls any initializer before constructing `FileManager`. This makes it impossible to tell the client which server to contact unless environment variables are set manually. 【F:main_fm.cpp†L1-L74】

## Why no files appear on the server
- Because the client connects to the wrong endpoint (or fails to connect at all), `sendCreate` never succeeds and `RemoteState::active` stays `false` in the constructor. Every RPC entry point checks `active` and returns immediately. As a result:
  - `writeFile` exits at the `if (!state.active) return;` guard, so the server receives no upload payload. 【F:client/fileManagerRemoto.cpp†L136-L152】
  - `listFiles` returns an empty vector after the same guard, so `lls` always prints nothing. 【F:client/fileManagerRemoto.cpp†L82-L110】
  - `readFile` exits before issuing `METHOD_READ`, so downloaded files are zero-length. 【F:client/fileManagerRemoto.cpp†L112-L134】

## Exact fixes with corrected code snippets
1. **Propagate host/port from the CLI into the remote connector.** Add a small shim that reads `argv` before `FileManager` construction and stores the values for `defaultHost()`/`defaultPort()` to use:
```cpp
// client/config.cpp
static std::string g_host = "127.0.0.1";
static int g_port = 5555;

void setConnectionTarget(const std::string& host, int port) {
    g_host = host; g_port = port;
}
std::string defaultHost() { return g_host; }
int defaultPort() { return g_port; }
```
Then create a thin `main` that forwards the arguments to `main_fm` logic before the first `FileManager` is built:
```cpp
// client/main.cpp
int main(int argc, char** argv) {
    if (argc == 3) setConnectionTarget(argv[1], std::atoi(argv[2]));
    return real_main_fm(argc, argv); // wrapper extracted from the provided main
}
```
(Alternatively, parse `argv` inside a static initializer used by `defaultHost/Port`.)

2. **Fail fast when the connection cannot be opened.** Have `sendCreate` return an explicit error and surface it to the user so they know the remote session is inactive, e.g.:
```cpp
if (!state.connection.connectTo(host, port)) {
    std::cerr << "Cannot connect to " << host << ':' << port << "\n";
    return;
}
if (!sendCreate(state, path)) {
    std::cerr << "Remote FileManager creation failed\n";
    return;
}
```

## Recommended protocol structure
- **Framing:** 4-byte length prefix in network byte order for every packet (already implemented by `sendPacket/recvPacket`).
- **Requests:**
  1. `METHOD_CREATE` (`uint32`), `pathLen` (`uint32`), `path` bytes.
  2. `METHOD_DESTROY`, `remoteId`.
  3. `METHOD_LIST`, `remoteId`.
  4. `METHOD_READ`, `remoteId`, `nameLen`, `name` bytes.
  5. `METHOD_WRITE`, `remoteId`, `nameLen`, `name` bytes, `dataLen`, `data` bytes.
- **Responses:**
  - Always start with `status` (`STATUS_OK`/`STATUS_ERROR`).
  - `CREATE` adds `remoteId`.
  - `DESTROY` echoes `remoteId`.
  - `LIST` adds `count`, then for each entry `nameLen` + `name`.
  - `READ` adds `dataLen` + `data`.
  - `WRITE` can simply return `STATUS_OK` (optional: echo bytes-written).

## Step-by-step fix plan
1. Add a lightweight CLI-aware entry point (or static initializer) that captures `argv[1]` and `argv[2]` and updates the host/port used by `defaultHost()`/`defaultPort()` before any `FileManager` is instantiated.
2. Replace the current `defaultHost/defaultPort` implementations with versions that read the captured values first and fall back to environment variables or the existing defaults.
3. Emit clear error messages when `connectTo` or `sendCreate` fail so the user knows uploads/listings are being skipped.
4. Re-test `upload`, `lls`, and `download` ensuring packets reach the server and the `FileManagerDir` contents change accordingly.
