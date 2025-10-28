# WiFi Stage 3: WpaEvent Backend Port - Implementation Plan

**Status:** READY FOR IMPLEMENTATION
**Estimated Time:** ~90 minutes
**Prerequisites:** Stage 2 complete (wpa_supplicant build system integration)

---

## Executive Summary

Port the parent GuppyScreen's `wpa_event.{h,cpp}` implementation to create `wifi_backend_wpa.{h,cpp}` - a libhv-based async wrapper for wpa_supplicant control interface.

**What we're building:**
- Linux: Real wpa_supplicant integration using libhv event loop
- macOS: Stub implementation for mock WiFi mode
- Architecture: Matches parent's proven patterns exactly

---

## Parent WpaEvent Architecture (Reference Implementation)

### File Locations
- **Header:** `../src/wpa_event.h`
- **Implementation:** `../src/wpa_event.cpp`

### Class Design

```cpp
class WpaEvent : private hv::EventLoopThread {
    // INHERITANCE: Private inheritance from libhv's event loop thread
    // Provides async Unix socket I/O without blocking

public:
    void start();      // Initialize wpa_supplicant connection
    void stop();       // Clean shutdown
    std::string send_command(const std::string& cmd);  // Sync commands
    void register_callback(const std::string& name,
                          std::function<void(const std::string&)>); // Event handlers

private:
    struct wpa_ctrl* conn;  // Control connection (commands)
    std::map<std::string, std::function<void(const std::string&)>> callbacks;

    void init_wpa();  // Runs in event loop thread
    void handle_wpa_events(void* data, int len);
    static void _handle_wpa_events(hio_t* io, void* data, int readbyte);  // Static trampoline
};
```

### Critical Patterns from Parent

#### Pattern 1: Two Connection Model

**Why two connections?** wpa_ctrl API design requires separate connections for commands vs. events.

```cpp
// Control connection: For synchronous commands
conn = wpa_ctrl_open(wpa_socket.c_str());

// Monitor connection: For async events (SEPARATE instance)
struct wpa_ctrl* mon_conn = wpa_ctrl_open(wpa_socket.c_str());
wpa_ctrl_attach(mon_conn);  // Subscribe to events
```

#### Pattern 2: Socket Discovery with P2P Filtering

```cpp
void init_wpa() {
    std::string wpa_socket = "/run/wpa_supplicant";

    // Auto-detect socket in directory
    if (fs::is_directory(fs::status(wpa_socket))) {
        for (const auto& e : fs::directory_iterator(wpa_socket)) {
            if (fs::is_socket(e.path())
                && e.path().string().find("p2p") == std::string::npos) {  // Skip P2P
                wpa_socket = e.path().string();
                break;
            }
        }
    }

    // Try alternative path if not found
    if (!fs::exists(wpa_socket)) {
        wpa_socket = "/var/run/wpa_supplicant";
        // ... repeat discovery
    }
}
```

**Known paths to try:**
1. `/run/wpa_supplicant/wlan0` (modern systemd)
2. `/var/run/wpa_supplicant/wlan0` (older systems)
3. Skip: `/run/wpa_supplicant/p2p-dev-wlan0` (P2P)

#### Pattern 3: libhv Event Loop Initialization

```cpp
void start() {
    if (isRunning()) {
        // Already running - schedule work in loop
        loop()->runInLoop([this]() { init_wpa_internal(); });
    } else {
        // Start new thread with initialization
        hv::EventLoopThread::start(true, [this]() {
            init_wpa_internal();
            return 0;
        });
    }
}

void init_wpa_internal() {
    // THIS RUNS IN EVENT LOOP THREAD

    // Open monitor connection
    struct wpa_ctrl *mon_conn = wpa_ctrl_open(wpa_socket.c_str());
    wpa_ctrl_attach(mon_conn);

    // Get file descriptor
    int monfd = wpa_ctrl_get_fd(mon_conn);

    // Register with libhv
    hio_t* io = hio_get(loop()->loop(), monfd);
    hio_set_context(io, this);
    hio_setcb_read(io, WpaEvent::_handle_wpa_events);
    hio_read_start(io);
}
```

#### Pattern 4: Static Callback Trampoline

**Why needed?** libhv uses C callbacks, needs static function to access instance.

```cpp
// Static method (can be C callback)
static void _handle_wpa_events(hio_t *io, void *data, int readbyte) {
    WpaEvent* wpa_event = (WpaEvent*)hio_context(io);  // Get instance pointer
    wpa_event->handle_wpa_events(data, readbyte);      // Call member
}

void handle_wpa_events(void* data, int len) {
    std::string event = std::string((char*)data, len);

    // Broadcast to ALL registered callbacks
    for (const auto& entry : callbacks) {
        entry.second(event);
    }
}
```

#### Pattern 5: Synchronous Command Sending

```cpp
std::string send_command(const std::string& cmd) {
    char resp[4096];
    size_t len = sizeof(resp) - 1;

    if (conn != NULL) {
        if (wpa_ctrl_request(conn, cmd.c_str(), cmd.length(),
                            resp, &len, NULL) == 0) {
            return std::string(resp, len);
        }
    }
    return "";
}
```

---

## Implementation Plan

### Phase 1: Create Header File (30 min)

**File:** `include/wifi_backend_wpa.h` (~150 lines)

#### Structure

```cpp
/*
 * Copyright (C) 2025 356C LLC
 * [GPL v3 header - see docs/COPYRIGHT_HEADERS.md]
 */

#pragma once

#include <string>
#include <functional>

#ifndef __APPLE__  // Linux implementation

#include "hv/EventLoop.h"
#include "hv/EventLoopThread.h"

struct wpa_ctrl;  // Forward declaration (from wpa_ctrl.h)

/**
 * @brief wpa_supplicant backend using libhv async event loop
 */
class WifiBackendWpa : private hv::EventLoopThread {
public:
    WifiBackendWpa();
    ~WifiBackendWpa();

    /**
     * @brief Initialize and connect to wpa_supplicant socket
     * @param socket_path Base path (e.g., "/run/wpa_supplicant")
     * @return true if connected successfully
     */
    bool init(const std::string& socket_path);

    /**
     * @brief Send synchronous command to wpa_supplicant
     * @param cmd Command string (e.g., "SCAN", "SCAN_RESULTS")
     * @return Response string from wpa_supplicant
     */
    std::string send_command(const std::string& cmd);

    /**
     * @brief Register callback for async events
     * @param cb Callback function receiving event string
     */
    void register_event_callback(std::function<void(const std::string&)> cb);

    void start();  ///< Start event monitoring loop
    void stop();   ///< Stop event monitoring loop

private:
    struct wpa_ctrl* ctrl_conn_;      // Command connection
    struct wpa_ctrl* monitor_conn_;   // Event monitoring connection
    std::function<void(const std::string&)> event_callback_;

    // libhv I/O callback (called when socket has data)
    static void handle_wpa_event_static(hio_t* io, void* data, int readbytes);
    void handle_wpa_event(void* data, int readbytes);

    // Socket path discovery
    std::string find_wpa_socket(const std::string& base_path);

    // Initialization (runs in event loop thread)
    void init_wpa_internal();
};

#else  // macOS - Mock stubs

class WifiBackendWpa {
public:
    WifiBackendWpa() {}
    ~WifiBackendWpa() {}
    bool init(const std::string&) { return false; }
    std::string send_command(const std::string&) { return ""; }
    void register_event_callback(std::function<void(const std::string&)>) {}
    void start() {}
    void stop() {}
};

#endif  // __APPLE__
```

**Key points:**
- GPL v3 copyright header (reference: `docs/COPYRIGHT_HEADERS.md`)
- Platform guards: `#ifndef __APPLE__` for real impl, `#else` for stubs
- Private inheritance from `hv::EventLoopThread`
- Two `wpa_ctrl*` members (control + monitor)
- Static trampoline for C callback
- Comprehensive Doxygen comments

---

### Phase 2: Create Implementation File (45 min)

**File:** `src/wifi_backend_wpa.cpp` (~200 lines)

#### Includes and Setup

```cpp
/*
 * Copyright (C) 2025 356C LLC
 * [GPL v3 header]
 */

#include "wifi_backend_wpa.h"
#include <spdlog/spdlog.h>

#ifndef __APPLE__  // Linux only

#include "wpa_ctrl.h"           // From wpa_supplicant submodule
#include "hv/hloop.h"           // libhv event loop

// Filesystem API (C++17 vs experimental)
#ifdef __APPLE__
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
```

#### Constructor/Destructor

```cpp
WifiBackendWpa::WifiBackendWpa()
    : hv::EventLoopThread(nullptr)
    , ctrl_conn_(nullptr)
    , monitor_conn_(nullptr) {
}

WifiBackendWpa::~WifiBackendWpa() {
    stop();  // 1. Stop event loop (joins thread)
    if (monitor_conn_) {
        wpa_ctrl_close(monitor_conn_);  // 2. Close monitor
        monitor_conn_ = nullptr;
    }
    if (ctrl_conn_) {
        wpa_ctrl_close(ctrl_conn_);     // 3. Close control
        ctrl_conn_ = nullptr;
    }
}
```

**CRITICAL ORDER:** Stop loop → close monitor → close control

#### Initialization

```cpp
bool WifiBackendWpa::init(const std::string& base_path) {
    std::string socket_path = find_wpa_socket(base_path);
    if (socket_path.empty()) {
        spdlog::error("[WiFi] No wpa_supplicant socket found in {}", base_path);
        return false;
    }

    // Open control connection
    ctrl_conn_ = wpa_ctrl_open(socket_path.c_str());
    if (!ctrl_conn_) {
        spdlog::error("[WiFi] Failed to open wpa_supplicant control: {}", socket_path);
        return false;
    }

    spdlog::info("[WiFi] Connected to wpa_supplicant: {}", socket_path);
    return true;
}
```

#### Socket Discovery

```cpp
std::string WifiBackendWpa::find_wpa_socket(const std::string& base_path) {
    // Check if base_path is already a socket
    if (fs::exists(base_path) && fs::is_socket(base_path)) {
        return base_path;
    }

    // Search directory for interface sockets (wlan0, wlan1, etc.)
    if (fs::is_directory(base_path)) {
        for (const auto& entry : fs::directory_iterator(base_path)) {
            if (fs::is_socket(entry.path())) {
                // Skip p2p sockets
                std::string name = entry.path().filename().string();
                if (name.find("p2p") == std::string::npos) {
                    spdlog::debug("[WiFi] Found socket: {}", entry.path().string());
                    return entry.path().string();
                }
            }
        }
    }

    return "";  // Not found
}
```

#### Event Loop Management

```cpp
void WifiBackendWpa::start() {
    if (isRunning()) {
        loop()->runInLoop([this]() { init_wpa_internal(); });
    } else {
        hv::EventLoopThread::start(true, [this]() {
            init_wpa_internal();
            return 0;
        });
    }
}

void WifiBackendWpa::stop() {
    hv::EventLoopThread::stop(true);
}

void WifiBackendWpa::init_wpa_internal() {
    // This runs in the event loop thread

    // Find socket (try common paths)
    std::string socket_path = find_wpa_socket("/run/wpa_supplicant");
    if (socket_path.empty()) {
        socket_path = find_wpa_socket("/var/run/wpa_supplicant");
    }

    if (socket_path.empty()) {
        spdlog::error("[WiFi] Cannot find wpa_supplicant socket for monitoring");
        return;
    }

    // Open monitor connection
    monitor_conn_ = wpa_ctrl_open(socket_path.c_str());
    if (!monitor_conn_) {
        spdlog::error("[WiFi] Failed to open monitor connection");
        return;
    }

    // Attach to receive events
    if (wpa_ctrl_attach(monitor_conn_) != 0) {
        spdlog::error("[WiFi] Failed to attach to wpa_supplicant");
        wpa_ctrl_close(monitor_conn_);
        monitor_conn_ = nullptr;
        return;
    }

    // Get socket file descriptor
    int fd = wpa_ctrl_get_fd(monitor_conn_);

    // Register with libhv event loop
    hio_t* io = hio_get(loop()->loop(), fd);
    if (!io) {
        spdlog::error("[WiFi] Failed to register socket with libhv");
        return;
    }

    hio_set_context(io, this);
    hio_setcb_read(io, WifiBackendWpa::handle_wpa_event_static);
    hio_read_start(io);

    spdlog::info("[WiFi] Event monitoring started");
}
```

#### Event Handling

```cpp
void WifiBackendWpa::handle_wpa_event_static(hio_t* io, void* data, int readbytes) {
    WifiBackendWpa* self = (WifiBackendWpa*)hio_context(io);
    self->handle_wpa_event(data, readbytes);
}

void WifiBackendWpa::handle_wpa_event(void* data, int readbytes) {
    std::string event((char*)data, readbytes);
    spdlog::debug("[WiFi] Event: {}", event);

    if (event_callback_) {
        event_callback_(event);
    }
}

void WifiBackendWpa::register_event_callback(std::function<void(const std::string&)> cb) {
    event_callback_ = cb;
}
```

#### Command Sending

```cpp
std::string WifiBackendWpa::send_command(const std::string& cmd) {
    if (!ctrl_conn_) {
        spdlog::warn("[WiFi] send_command called but not connected");
        return "";
    }

    char reply[4096];
    size_t reply_len = sizeof(reply) - 1;

    spdlog::debug("[WiFi] Command: {}", cmd);

    if (wpa_ctrl_request(ctrl_conn_, cmd.c_str(), cmd.length(),
                         reply, &reply_len, nullptr) == 0) {
        reply[reply_len] = '\0';
        return std::string(reply, reply_len);
    }

    spdlog::error("[WiFi] Command failed: {}", cmd);
    return "";
}

#endif  // __APPLE__
```

---

### Phase 3: Verification (15 min)

#### Build Tests

**macOS:**
```bash
make clean && make -j8
# Should compile stub class without errors
# No wpa_supplicant dependencies
```

**Linux:**
```bash
make clean && make -j8
# Should build libwpa_client.a (220K)
# Should compile wifi_backend_wpa.cpp
# Should link without undefined symbols
```

#### Code Review Checks

- [ ] All new code has GPL v3 headers
- [ ] All logging uses spdlog (no printf/cout)
- [ ] Platform guards work correctly (`#ifndef __APPLE__`)
- [ ] No compiler warnings
- [ ] Matches parent's architectural patterns
- [ ] Destructor has correct order (stop → monitor → control)

---

## Risks and Mitigations

### Risk 1: Thread Safety (CRITICAL for Stage 4)

**Problem:** libhv event loop runs in separate thread from LVGL

**Solution (to implement in Stage 4):**

```cpp
// Global event queue (thread-safe)
static std::mutex wifi_event_mutex;
static std::queue<std::string> wifi_event_queue;

// libhv thread: Enqueue events
static void handle_wpa_event(const std::string& event) {
    std::lock_guard<std::mutex> lock(wifi_event_mutex);
    wifi_event_queue.push(event);
}

// LVGL thread: Process events
static void process_wifi_events_timer(lv_timer_t* timer) {
    std::lock_guard<std::mutex> lock(wifi_event_mutex);
    while (!wifi_event_queue.empty()) {
        std::string event = wifi_event_queue.front();
        wifi_event_queue.pop();

        // NOW safe to update LVGL widgets
        if (event.find("SCAN-RESULTS") != std::string::npos) {
            // Update network list UI
        }
    }
}
```

**Reference:** Migration guide lines 1115-1141

### Risk 2: wpa_supplicant Not Running

**Scenarios:**
1. Daemon not installed → Fallback to mock mode, warn
2. Daemon crashed → Detect on command failure
3. Socket permissions → Log error, suggest `usermod -aG netdev`

**Graceful degradation:**
```cpp
bool init() {
    if (socket_path.empty()) {
        spdlog::warn("[WiFi] wpa_supplicant socket not found - WiFi disabled");
        return false;  // WiFiManager falls back to mock
    }
}
```

### Risk 3: Filesystem API Differences

**Issue:** C++17 `<filesystem>` vs pre-C++17 `<experimental/filesystem>`

**Solution:** Use namespace alias (parent's pattern):
```cpp
#ifdef __APPLE__
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
```

### Risk 4: Memory Leaks

**Critical points:**
- `wpa_ctrl_open()` → MUST `wpa_ctrl_close()` in destructor
- Monitor connection → MUST stop event loop BEFORE closing
- libhv thread → MUST join before destroying connections

**Correct destructor order (already in plan):**
1. Stop event loop (joins thread)
2. Close monitor connection
3. Close control connection

---

## Dependencies

### Required Headers

```cpp
#include "wifi_backend_wpa.h"
#include <spdlog/spdlog.h>

#ifndef __APPLE__
#include "wpa_ctrl.h"           // From wpa_supplicant submodule
#include "hv/hloop.h"           // libhv event loop
#include "hv/EventLoop.h"       // C++ wrapper
#include "hv/EventLoopThread.h" // Thread wrapper
#include <filesystem>           // or <experimental/filesystem>
#endif
```

### Build System Integration

**Already configured (Stage 2 complete):**
- `WPA_INC`: `-I$(WPA_DIR)/src/common -I$(WPA_DIR)/src/utils`
- `LIBHV_INC`: `-I$(LIBHV_DIR)/include -I$(LIBHV_DIR)/cpputil`
- `LDFLAGS` (Linux): Includes `$(WPA_CLIENT_LIB)`
- `LDFLAGS` (macOS): Excludes wpa libraries

---

## Integration Points (Preview of Stage 4)

**Where WifiBackendWpa will be used:**

```cpp
// In wifi_manager.cpp
#ifndef __APPLE__
#include "wifi_backend_wpa.h"
static WifiBackendWpa wpa_backend;
static bool wpa_initialized = false;
#endif

// WiFiManager::set_enabled()
if (!wpa_initialized) {
    wpa_initialized = wpa_backend.init("/run/wpa_supplicant");
    if (!wpa_initialized) {
        wpa_initialized = wpa_backend.init("/var/run/wpa_supplicant");
    }

    wpa_backend.register_event_callback([](const std::string& event) {
        // Enqueue for LVGL thread processing (Stage 4)
    });
}

if (enabled) {
    wpa_backend.start();
} else {
    wpa_backend.stop();
}

// WiFiManager::perform_scan()
std::string result = wpa_backend.send_command("SCAN");
// Wait for CTRL-EVENT-SCAN-RESULTS async event
std::string results = wpa_backend.send_command("SCAN_RESULTS");
// Parse tab-separated format
```

---

## Success Criteria

- ✅ **Compiles on macOS** (stubs only, no warnings)
- ✅ **Compiles on Linux** (real implementation, links libwpa_client.a)
- ✅ **No runtime errors** when included in build
- ✅ **Matches parent patterns** (architecture, naming, design)
- ✅ **Ready for Stage 4** (WiFiManager integration)

---

## Next Steps After Stage 3

**Stage 4: WiFiManager Integration** (~60 min)
- Add thread-safe event queue (mutex + std::queue)
- Implement real scan using wpa_backend.send_command("SCAN")
- Implement connect using ADD_NETWORK → SET → SELECT pattern
- Parse SCAN_RESULTS tab-separated format

**Stage 5: Testing** (~45 min)
- Test with real wpa_supplicant daemon on Linux
- Verify scan returns real networks
- Test WPA2 connection flow
- Error scenario testing

**Stage 6: Documentation** (~20 min)
- Update CLAUDE.md with wpa_supplicant build requirements
- Update BUILD_SYSTEM.md with wpa_supplicant section
- Create docs/WIFI_SETUP.md for daemon setup

---

## References

**Parent codebase:**
- `../src/wpa_event.{h,cpp}` - Reference implementation
- `../src/config.cpp` - Socket path configuration

**Current implementation:**
- `include/wifi_manager.h` - Public WiFi API
- `src/wifi_manager.cpp` - Current mock implementation

**Documentation:**
- `docs/WIFI_WPA_SUPPLICANT_MIGRATION.md` - Comprehensive migration guide
- `wpa_supplicant/src/common/wpa_ctrl.h` - API reference

**Build system:**
- `Makefile` lines 113-134 - wpa_supplicant variables
- `mk/deps.mk` - wpa_supplicant build target

---

**Document created:** 2025-10-28
**Status:** READY FOR IMPLEMENTATION
**Estimated effort:** 90 minutes (backend only)
