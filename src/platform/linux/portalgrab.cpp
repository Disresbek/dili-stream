/**
 * @file src/platform/linux/portalgrab.cpp
 * @brief Definitions for XDG portal grab.
 */
// local includes
#include "pipewire.cpp"
#include "src/config.h"
#include "src/globals.h"

// standard includes
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>

// lib includes
#include <nlohmann/json.hpp>

namespace {
  // Portal configuration constants
  constexpr uint32_t SOURCE_TYPE_MONITOR = 1;
  constexpr uint32_t SOURCE_TYPE_VIRTUAL = 4;
  constexpr uint32_t CURSOR_MODE_EMBEDDED = 2;

  constexpr uint32_t PERSIST_FORGET = 0;
  constexpr uint32_t PERSIST_WHILE_RUNNING = 1;
  constexpr uint32_t PERSIST_UNTIL_REVOKED = 2;

  constexpr uint32_t TYPE_KEYBOARD = 1;
  constexpr uint32_t TYPE_POINTER = 2;
  constexpr uint32_t TYPE_TOUCHSCREEN = 4;

  // Portal D-Bus interface names and paths
  constexpr const char *PORTAL_NAME = "org.freedesktop.portal.Desktop";
  constexpr const char *PORTAL_PATH = "/org/freedesktop/portal/desktop";
  constexpr const char *REMOTE_DESKTOP_IFACE = "org.freedesktop.portal.RemoteDesktop";
  constexpr const char *SCREENCAST_IFACE = "org.freedesktop.portal.ScreenCast";
  constexpr const char *REQUEST_IFACE = "org.freedesktop.portal.Request";

  constexpr const char REQUEST_PREFIX[] = "/org/freedesktop/portal/desktop/request/";
  constexpr const char SESSION_PREFIX[] = "/org/freedesktop/portal/desktop/session/";
}  // namespace

using namespace std::literals;

namespace portal {
  /**
   * @brief Virtual display support (Apollo-style) for KDE Plasma.
   *
   * Enabled by setting output_name = virtual in sunshine.conf.
   * The portal then creates a new virtual screen for every stream instead of
   * capturing an existing monitor. Afterwards the screen is switched to the
   * client's resolution and refresh rate with kscreen-doctor (custom modes).
   */
  namespace virtual_display {
    /**
     * @brief A display mode of a KDE output.
     */
    struct display_mode_t {
      std::string id;  ///< kscreen mode id.
      int width = 0;  ///< Width in pixels.
      int height = 0;  ///< Height in pixels.
      double hz = 0;  ///< Refresh rate in Hz.
    };

    /**
     * @brief A KDE output as reported by kscreen-doctor.
     */
    struct output_t {
      int id = 0;  ///< kscreen output id (only valid within one kscreen-doctor run).
      std::string name;  ///< Connector name.
      std::string uuid;  ///< Stable output uuid, if reported.
      int priority = 0;  ///< 1 = main screen.
      std::string current_mode;  ///< Active mode id.
      std::vector<display_mode_t> modes;  ///< Available modes.

      /**
       * @brief How to address this output in a kscreen-doctor command.
       *
       * kscreen-doctor splits commands at dots, so names containing dots (like the
       * portal's "Virtual-virtual-xdp-kde-org.kde...") cannot be used directly.
       */
      std::string selector() const {
        if (name.find('.') == std::string::npos && !name.empty()) {
          return name;
        }
        if (!uuid.empty()) {
          return uuid;
        }
        return std::to_string(id);
      }
    };

    /**
     * @brief Whether virtual display mode is enabled in the configuration.
     */
    inline bool enabled() {
      return config::video.output_name == "virtual";
    }

    /**
     * @brief Prefix needed to run a command on the host (Flatpak, distrobox or native).
     */
    inline std::string host_prefix() {
      std::error_code ec;
      if (std::filesystem::exists("/.flatpak-info", ec)) {
        return "flatpak-spawn --host ";
      }
      if (std::getenv("CONTAINER_ID") && std::filesystem::exists("/usr/bin/distrobox-host-exec", ec)) {
        return "distrobox-host-exec ";
      }
      return "";
    }

    /**
     * @brief Environment kscreen-doctor needs to reach the KDE session.
     *
     * Commands started through distrobox-host-exec or flatpak-spawn do not inherit the
     * Wayland session variables, so Qt would fall back to X11 and fail.
     */
    inline std::string host_env() {
      std::string env = "env QT_QPA_PLATFORM=wayland";
      for (const char *var : {"WAYLAND_DISPLAY", "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS"}) {
        if (const char *value = std::getenv(var)) {
          env += std::format(" {}={}", var, value);
        }
      }
      return env + " ";
    }

    /**
     * @brief Run a command on the host (inside the KDE session) and return its output.
     */
    inline std::string run_host(const std::string &command) {
      const std::string cmd = host_prefix() + host_env() + command + " 2>/dev/null";
      BOOST_LOG(debug) << "[virtual_display] Running: "sv << cmd;
      std::string out;
      FILE *pipe = popen(cmd.c_str(), "r");
      if (!pipe) {
        BOOST_LOG(error) << "[virtual_display] Could not run kscreen-doctor"sv;
        return out;
      }
      std::array<char, 4096> buf {};
      while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        out += buf.data();
      }
      pclose(pipe);
      return out;
    }

    /**
     * @brief Run kscreen-doctor on the host and return its output.
     */
    inline std::string kscreen(const std::string &args) {
      return run_host("kscreen-doctor " + args);
    }

    constexpr const char *KWIN_SCRIPT_NAME = "sunshine_virtual_display";

    /**
     * @brief KWin script that moves windows onto the virtual screen while it exists.
     *
     * New windows, and windows you switch to (for example Steam when it is already open),
     * are sent to the streamed screen, so they are always visible on the device.
     */
    constexpr const char *KWIN_SCRIPT = R"js(
// Sunshine virtual display: keep the stream's windows on the streamed screen,
// without taking windows away from someone using the PC at the same time.
//
// Rule: whatever is opened or clicked while the mouse pointer is on the streamed
// screen belongs to the stream. Everything else stays where it is.
function virtualOutput() {
  const screens = workspace.screens;
  for (let i = 0; i < screens.length; i++) {
    if (screens[i].name.startsWith("Virtual-")) {
      return screens[i];
    }
  }
  return null;
}

function pointerOn(output) {
  const p = workspace.cursorPos;
  const g = output.geometry;
  return p.x >= g.x && p.x < g.x + g.width && p.y >= g.y && p.y < g.y + g.height;
}

function eligible(window) {
  // Skip panels, notifications, popups, dialogs attached to another window, etc.
  return window && !window.specialWindow && !window.transient && !window.popupWindow;
}

function bringBack(window) {
  // Only for windows that belong to the stream: games and Steam Big Picture often
  // jump to another screen when they switch to fullscreen. Move them back.
  const output = virtualOutput();
  if (!output || !window.sunshineStream || window.output === output) {
    return;
  }
  window.sunshineMoves = (window.sunshineMoves || 0) + 1;
  if (window.sunshineMoves > 10) {
    return;  // Never fight an application forever
  }
  print("sunshine: bringing '" + window.caption + "' back to " + output.name);
  workspace.sendClientToScreen(window, output);
}

function claim(window) {
  if (!eligible(window)) {
    return;
  }
  const output = virtualOutput();
  if (!output) {
    return;
  }
  if (!window.sunshineStream && !pointerOn(output)) {
    return;  // Opened or clicked at the PC: leave it alone
  }
  if (!window.sunshineStream) {
    window.sunshineStream = true;
    window.fullScreenChanged.connect(function () { bringBack(window); });
    window.outputChanged.connect(function () { bringBack(window); });
  }
  if (window.output !== output) {
    print("sunshine: moving '" + window.caption + "' to " + output.name);
    workspace.sendClientToScreen(window, output);
  }
}

workspace.windowAdded.connect(claim);
workspace.windowActivated.connect(claim);
)js";

    /**
     * @brief Load the window-moving KWin script.
     */
    inline void load_window_script() {
      const auto dir = std::filesystem::path(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") / ".local/share/sunshine";
      std::error_code ec;
      std::filesystem::create_directories(dir, ec);
      const auto path = dir / "virtual_display_windows.js";
      {
        std::ofstream file(path);
        file << KWIN_SCRIPT;
      }
      const std::string dbus = "dbus-send --session --print-reply --dest=org.kde.KWin /Scripting ";
      run_host(dbus + "org.kde.kwin.Scripting.unloadScript string:" + KWIN_SCRIPT_NAME);
      run_host(dbus + "org.kde.kwin.Scripting.loadScript string:" + path.string() + " string:" + KWIN_SCRIPT_NAME);
      run_host(dbus + "org.kde.kwin.Scripting.start");
      BOOST_LOG(info) << "[virtual_display] New windows will open on the virtual screen"sv;
    }

    /**
     * @brief Unload the window-moving KWin script.
     */
    inline void unload_window_script() {
      run_host(std::string("dbus-send --session --print-reply --dest=org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript string:") + KWIN_SCRIPT_NAME);
    }

    /**
     * @brief Read all outputs from KDE.
     */
    inline std::vector<output_t> read_outputs() {
      std::vector<output_t> result;
      const auto text = kscreen("-j");
      try {
        auto json = nlohmann::json::parse(text);
        for (const auto &o : json.at("outputs")) {
          output_t out;
          out.id = o.value("id", 0);
          out.name = o.value("name", "");
          out.uuid = o.value("uuid", "");
          out.priority = o.value("priority", 0);
          if (o.contains("currentModeId")) {
            const auto &cm = o.at("currentModeId");
            out.current_mode = cm.is_string() ? cm.get<std::string>() : cm.dump();
          }
          for (const auto &m : o.value("modes", nlohmann::json::array())) {
            display_mode_t mode;
            const auto &mid = m.at("id");
            mode.id = mid.is_string() ? mid.get<std::string>() : mid.dump();
            mode.width = m.at("size").value("width", 0);
            mode.height = m.at("size").value("height", 0);
            mode.hz = m.value("refreshRate", 0.0);
            out.modes.push_back(mode);
          }
          result.push_back(out);
        }
      } catch (const std::exception &e) {
        BOOST_LOG(error) << "[virtual_display] Could not read kscreen-doctor output: "sv << e.what();
        BOOST_LOG(error) << "[virtual_display] Raw output (first 200 characters): '"sv << text.substr(0, 200) << "'"sv;
      }
      return result;
    }

    /**
     * @brief Find the mode closest to what the client asked for.
     *
     * KDE may round the width to a multiple of 8, so a few pixels difference is accepted.
     */
    inline std::optional<display_mode_t> find_mode(const output_t &out, int width, int height, int fps) {
      std::optional<display_mode_t> best;
      for (const auto &m : out.modes) {
        if (m.height != height || std::abs(m.width - width) > 8) {
          continue;
        }
        if (!best || std::abs(m.hz - fps) < std::abs(best->hz - fps)) {
          best = m;
        }
      }
      if (best && std::abs(best->hz - fps) < 1.0) {
        return best;
      }
      return std::nullopt;
    }

    /**
     * @brief Ids of all outputs that currently exist.
     */
    inline std::set<int> output_ids() {
      std::set<int> ids;
      for (const auto &o : read_outputs()) {
        ids.insert(o.id);
      }
      return ids;
    }

    /**
     * @brief State needed to undo our changes when the stream ends.
     */
    struct session_t {
      std::string output;  ///< Selector of our virtual output.
      std::string previous_primary;  ///< Selector of the output that was the main screen before.
      int width = 0;  ///< Final width.
      int height = 0;  ///< Final height.
      bool made_primary = false;  ///< Whether we made the virtual screen the main screen.
    };

    /**
     * @brief Find the new virtual output and switch it to the client's mode.
     *
     * @return The session state, or nullopt when no new virtual output was found.
     */
    inline std::optional<session_t> apply(int width, int height, int fps) {
      std::optional<output_t> target;
      std::string previous_primary;
      int best_priority = 0;
      // Wait up to 2 seconds for KDE to report the new screen
      for (int attempt = 0; attempt < 20 && !target; ++attempt) {
        const auto outputs = read_outputs();
        if (outputs.empty()) {
          // kscreen-doctor is not working at all, waiting longer will not help
          BOOST_LOG(warning) << "[virtual_display] kscreen-doctor returned nothing, skipping resolution matching"sv;
          return std::nullopt;
        }
        for (const auto &o : outputs) {
          // Remember the physical screen with the highest rank (lowest priority number)
          if (!o.name.starts_with("Virtual-") && o.priority > 0 && (best_priority == 0 || o.priority < best_priority)) {
            best_priority = o.priority;
            previous_primary = o.selector();
          }
          // Screens created by the portal are called "Virtual-virtual-xdp-..."
          if (o.name.starts_with("Virtual-") && o.name.find("xdp") != std::string::npos) {
            target = o;
          }
        }
        if (!target) {
          std::this_thread::sleep_for(100ms);
        }
      }
      if (!target) {
        BOOST_LOG(warning) << "[virtual_display] No new virtual screen found"sv;
        return std::nullopt;
      }
      const auto id = target->selector();
      BOOST_LOG(info) << "[virtual_display] Virtual screen '"sv << target->name << "' (id "sv << id << "), client wants "sv << width << "x"sv << height << " at "sv << fps << " Hz"sv;

      // Reuse a matching mode, or add a custom one (any refresh rate works this way)
      auto mode = find_mode(*target, width, height, fps);
      if (!mode) {
        kscreen(std::format("output.{}.addCustomMode.{}.{}.{}.full", id, width, height, fps * 1000));
        for (const auto &o : read_outputs()) {
          if (o.name == target->name) {
            target = o;
          }
        }
        mode = find_mode(*target, width, height, fps);
      }

      session_t session {.output = id, .previous_primary = previous_primary, .width = width, .height = height};
      if (mode) {
        if (mode->id != target->current_mode) {
          kscreen(std::format("output.{}.mode.{}", id, mode->id));
          // Wait until KDE really runs the new mode, so the capture starts at the right size
          for (int attempt = 0; attempt < 15; ++attempt) {
            bool applied = false;
            for (const auto &o : read_outputs()) {
              if (o.name == target->name && o.current_mode == mode->id) {
                applied = true;
              }
            }
            if (applied) {
              break;
            }
            std::this_thread::sleep_for(100ms);
          }
        }
        session.width = mode->width;
        session.height = mode->height;
        BOOST_LOG(info) << "[virtual_display] Running at "sv << mode->width << "x"sv << mode->height << " at "sv << mode->hz << " Hz"sv;
      } else {
        BOOST_LOG(warning) << "[virtual_display] Could not create a matching mode, keeping KDE's default"sv;
        for (const auto &m : target->modes) {
          if (m.id == target->current_mode) {
            session.width = m.width;
            session.height = m.height;
          }
        }
      }

      if (config::video.virtual_display_primary) {
        // Setting on: make it the main screen so games and new windows open there
        if (target->priority != 1) {
          kscreen(std::format("output.{}.priority.1", id));
        }
        session.made_primary = true;
        BOOST_LOG(info) << "[virtual_display] Virtual screen is now the main screen"sv;
      } else if (target->priority == 1 && !previous_primary.empty()) {
        // Setting off, but KDE remembered an older "main screen" choice: give it back to the monitor
        kscreen(std::format("output.{}.priority.1", previous_primary));
        BOOST_LOG(info) << "[virtual_display] Keeping '"sv << previous_primary << "' as the main screen"sv;
      }

      // Give KWin a moment to settle before PipeWire negotiates the stream
      std::this_thread::sleep_for(300ms);
      return session;
    }

    /**
     * @brief Give the main screen back to the monitor that had it before.
     */
    inline void restore(const session_t &session) {
      if (session.made_primary && !session.previous_primary.empty() && session.previous_primary != session.output) {
        kscreen(std::format("output.{}.priority.1", session.previous_primary));
        BOOST_LOG(info) << "[virtual_display] Main screen restored"sv;
      }
    }
  }  // namespace virtual_display
  // Forward declarations
  class runtime_t;

  /**
   * @brief Persistent portal restore token used to reuse screencast permission.
   */
  class restore_token_t {
  public:
    /**
     * @brief Return the currently wrapped value or handle.
     *
     * @return Underlying native handle or object pointer.
     */
    static std::string get() {
      return *token_;
    }

    /**
     * @brief Store the new value and mark it dirty for persistence.
     *
     * @param value Portal restore token received from xdg-desktop-portal.
     */
    static void set(std::string_view value) {
      *token_ = value;
    }

    /**
     * @brief Return whether the persisted value is empty.
     *
     * @return True when no portal display id has been persisted.
     */
    static bool empty() {
      return token_->empty();
    }

    /**
     * @brief Load persisted state from its backing store.
     */
    static void load() {
      std::ifstream file(get_file_path());
      if (file.is_open()) {
        std::getline(file, *token_);
        if (!token_->empty()) {
          BOOST_LOG(info) << "[portalgrab] Loaded portal restore token from disk"sv;
        }
      }
    }

    /**
     * @brief Check if a Portal restore token exists on disk without inspecting its contents.
     *
     * @return True if file exists on disk.
     */
    static bool exists() {
      std::error_code ec;
      return std::filesystem::exists(get_file_path(), ec);
    }

    /**
     * @brief Clear a restore token if it already exists on disk.
     */
    static void clear() {
      std::error_code ec;
      token_->clear();
      std::filesystem::remove(get_file_path(), ec);
    }

    /**
     * @brief Save current state to its backing store.
     */
    static void save() {
      if (token_->empty()) {
        return;
      }
      std::ofstream file(get_file_path());
      if (file.is_open()) {
        file << *token_;
        BOOST_LOG(info) << "[portalgrab] Saved portal restore token to disk"sv;
      } else {
        BOOST_LOG(warning) << "[portalgrab] Failed to save portal restore token"sv;
      }
    }

  private:
    static inline const std::unique_ptr<std::string> token_ = std::make_unique<std::string>();

    static std::string get_file_path() {
      return platf::appdata().string() + "/portal_token";
    }
  };

  /**
   * @brief Clear a restore token if it already exists on disk.
   */
  void clear_saved_token() {
    restore_token_t::clear();
  }

  /**
   * @brief Check if a Portal restore token exists on disk without inspecting its contents.
   *
   * @return True if a saved token was found.
   */
  bool has_saved_token() {
    return restore_token_t::exists();
  }

  /**
   * @brief Check if the Portal service responds to a DBus Ping within 2 seconds.
   *
   * @return True if the Portal is reachable.
   */
  bool is_portal_service_reachable() {
    g_autoptr(GError) g_error = nullptr;
    g_autofree const gchar *address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, nullptr, &g_error);
    if (!address) {
      return false;
    }

    g_autoptr(GError) ping_error = nullptr;
    g_autoptr(GDBusConnection) conn = g_dbus_connection_new_for_address_sync(
      address,
      GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
      nullptr,
      nullptr,
      &ping_error
    );
    if (!conn) {
      return false;
    }

    g_autoptr(GVariant) reply = g_dbus_connection_call_sync(
      conn,
      "org.freedesktop.portal.Desktop",
      "/org/freedesktop/portal/desktop",
      "org.freedesktop.DBus.Peer",
      "Ping",
      nullptr,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      2000,
      nullptr,
      &ping_error
    );
    return reply != nullptr;
  }

  /**
   * @brief DBus response loop and response variant for portal calls.
   */
  struct dbus_response_t {
    GMainLoop *loop;  ///< GLib main loop waiting for a portal response signal.
    GVariant *response;  ///< DBus response payload returned by the portal.
    guint subscription_id;  ///< Subscription ID.
    std::string request_path;  ///< For Request.Close() on cancellation.
    GDBusConnection *conn;  ///< Borrowed — owned by the calling dbus_t/portal_t.
  };

  /**
   * @brief PipeWire stream node and negotiated capture size.
   */
  struct pipewire_streaminfo_t {
    uint32_t pipewire_node = PW_ID_ANY;  ///< PipeWire node ID selected by the portal.
    uint64_t pipewire_object_serial = SPA_ID_INVALID;  ///< PipeWire object serial selected by the portal.
    int width = 0;  ///< Stream width in pixels.
    int height = 0;  ///< Stream height in pixels.
    int pos_x = 0;  ///< Output X position reported by the portal.
    int pos_y = 0;  ///< Output Y position reported by the portal.
    std::string monitor_name;  ///< Monitor name.

    /**
     * @brief Convert to display name.
     *
     * @return Value converted to display name.
     */
    std::string to_display_name() {
      if (!monitor_name.empty()) {
        return monitor_name;
      }
      return std::format("position-{}x{}-resolution-{}x{}", pos_x, pos_y, width, height);
    }

    /**
     * @brief Check whether a portal stream matches a requested display name.
     *
     * @param display_name Display name.
     * @return True when the portal display id matches the requested display name.
     */
    bool match_display_name(const std::string_view &display_name) {
      // Check the given non-empty display name matches the display name for this struct
      return !display_name.empty() && display_name == to_display_name();
    }
  };

  /**
   * @brief DBus connection and portal request helpers for screencast setup.
   */
  class dbus_t {
  public:
    guint dbus_timeout = 10;  ///< Timeout in seconds for DBus calls.

    dbus_t &operator=(dbus_t &&) = delete;  // Do not allow to copying

    ~dbus_t() noexcept {
      try {
        if (conn && !session_handle.empty()) {
          g_autoptr(GError) err = nullptr;
          // This is a blocking C call; it won't throw, but we wrap for safety
          g_dbus_connection_call_sync(
            conn,
            "org.freedesktop.portal.Desktop",
            session_handle.c_str(),
            "org.freedesktop.portal.Session",
            "Close",
            nullptr,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            dbus_timeout * 1000,
            nullptr,
            &err
          );

          if (err) {
            BOOST_LOG(warning) << "[portalgrab] Failed to explicitly close portal session: "sv << err->message;
          } else {
            BOOST_LOG(debug) << "[portalgrab] Explicitly closed portal session: "sv << session_handle;
          }
        }
      } catch (const std::exception &e) {
        BOOST_LOG(error) << "[portalgrab] Standard exception caught in ~dbus_t: "sv << e.what();
      } catch (...) {
        BOOST_LOG(error) << "[portalgrab] Unknown exception caught in ~dbus_t"sv;
      }

      if (pipewire_fd >= 0) {
        close(pipewire_fd);
      }
      if (screencast_proxy) {
        g_clear_object(&screencast_proxy);
      }
      if (remote_desktop_proxy) {
        g_clear_object(&remote_desktop_proxy);
      }
      if (conn) {
        g_clear_object(&conn);
      }
    }

    /**
     * @brief Open DBus and prepare portal screencast request handling.
     *
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init() {
      restore_token_t::load();

      g_autoptr(GError) g_error = nullptr;
      g_autofree gchar *address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, nullptr, &g_error);
      if (!address) {
        return -1;
      }

      conn = g_dbus_connection_new_for_address_sync(
        address,
        GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
        nullptr,
        nullptr,
        &g_error
      );
      if (!conn) {
        return -1;
      }

      remote_desktop_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        PORTAL_NAME,
        PORTAL_PATH,
        REMOTE_DESKTOP_IFACE,
        nullptr,
        &g_error
      );
      if (!remote_desktop_proxy) {
        return -1;
      }

      screencast_proxy = g_dbus_proxy_new_sync(
        conn,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        PORTAL_NAME,
        PORTAL_PATH,
        SCREENCAST_IFACE,
        nullptr,
        &g_error
      );
      if (!screencast_proxy) {
        return -1;
      }

      return 0;
    }

    /**
     * @brief Connect to xdg-desktop-portal and restore or create a screencast session.
     *
     * @param allow_start_timeout True if "Start" DBus call is allowed to time out.
     * @return 0 when a portal session is ready; nonzero when D-Bus or portal setup fails.
     */
    int connect_to_portal(bool allow_start_timeout) {
      g_autoptr(GMainContext) context = g_main_context_new();
      g_autoptr(GMainLoop) loop = g_main_loop_new(context, false);
      g_autofree gchar *session_path = nullptr;
      g_autofree gchar *session_token = nullptr;
      create_session_path(conn, nullptr, &session_token);

      // Try combined RemoteDesktop + ScreenCast session first
      bool use_screencast_only = !try_remote_desktop_session(loop, &session_path, session_token);

      // Fall back to ScreenCast-only if RemoteDesktop failed
      if (use_screencast_only && try_screencast_only_session(loop, &session_path) < 0) {
        return -1;
      }

      if (start_portal_session(loop, session_path, pipewire_streams, use_screencast_only, allow_start_timeout) < 0) {
        return -1;
      }

      if (open_pipewire_remote(session_path, pipewire_fd) < 0) {
        return -1;
      }

      return 0;
    }

    // Try to create a combined RemoteDesktop + ScreenCast session
    // Returns true on success, false if should fall back to ScreenCast-only
    /**
     * @brief Try to create a RemoteDesktop portal session.
     *
     * @param loop GLib main loop associated with the portal request.
     * @param session_path Session path.
     * @param session_token Session token.
     * @return True when the portal request or state check succeeds.
     */
    bool try_remote_desktop_session(GMainLoop *loop, gchar **session_path, const gchar *session_token) {
      if (create_portal_session(loop, session_path, session_token, false) < 0) {
        return false;
      }

      if (select_remote_desktop_devices(loop, *session_path) < 0) {
        BOOST_LOG(warning) << "[portalgrab] RemoteDesktop.SelectDevices failed, falling back to ScreenCast-only mode"sv;
        g_free(*session_path);
        *session_path = nullptr;
        return false;
      }

      if (select_screencast_sources(loop, *session_path, false) < 0) {
        BOOST_LOG(warning) << "[portalgrab] ScreenCast.SelectSources failed with RemoteDesktop session, trying ScreenCast-only mode"sv;
        g_free(*session_path);
        *session_path = nullptr;
        return false;
      }

      return true;
    }

    // Create a ScreenCast-only session
    /**
     * @brief Create a screencast-only portal session without remote-desktop control.
     *
     * @param loop GLib main loop associated with the portal request.
     * @param session_path Session path.
     * @return 0 when the portal returns a session path; nonzero on request failure.
     */
    int try_screencast_only_session(GMainLoop *loop, gchar **session_path) {
      g_autofree gchar *new_session_token = nullptr;
      create_session_path(conn, nullptr, &new_session_token);
      if (create_portal_session(loop, session_path, new_session_token, true) < 0) {
        return -1;
      }
      if (select_screencast_sources(loop, *session_path, true) < 0) {
        g_free(*session_path);
        *session_path = nullptr;
        return -1;
      }
      return 0;
    }

    /**
     * @brief Check whether session closed.
     *
     * @return True when the portal session has been closed.
     */
    bool is_session_closed() const {
      if (conn && !session_handle.empty()) {
        // Try to retrieve property org.freedesktop.portal.Session::version
        g_autoptr(GError) err = nullptr;
        g_dbus_connection_call_sync(
          conn,
          "org.freedesktop.portal.Desktop",
          session_handle.c_str(),
          "org.freedesktop.DBus.Properties",
          "Get",
          g_variant_new("(ss)", "org.freedesktop.portal.Session", "version"),
          G_VARIANT_TYPE("(v)"),
          G_DBUS_CALL_FLAGS_NONE,
          dbus_timeout * 1000,
          nullptr,
          &err
        );
        // If we cannot get the property then the session portal was closed.
        if (err) {
          BOOST_LOG(debug) << "[portalgrab] Session closed as check failed: "sv << err->message;
          return true;
        }
      }
      // The session is not closed (or might not have been opened yet).
      return false;
    }

    /**
     * @brief Open a new PipeWire connection for an already running portal session.
     *
     * @return 0 on success; negative on failure.
     */
    int reopen_pipewire_remote() {
      return open_pipewire_remote(session_handle.c_str(), pipewire_fd);
    }

    std::vector<pipewire_streaminfo_t> pipewire_streams;  ///< Pipewire streams.
    int pipewire_fd = -1;  ///< Pipewire fd.

  private:
    GDBusConnection *conn;
    GDBusProxy *screencast_proxy;
    GDBusProxy *remote_desktop_proxy;
    std::string session_handle;

    int create_portal_session(GMainLoop *loop, gchar **session_path_out, const gchar *session_token, bool use_screencast) {
      GDBusProxy *proxy = use_screencast ? screencast_proxy : remote_desktop_proxy;
      const char *session_type = use_screencast ? "ScreenCast" : "RemoteDesktop";

      dbus_response_t response {};
      g_autofree gchar *request_token = nullptr;
      create_request_path(conn, nullptr, &request_token);

      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("(a{sv})"));
      g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(request_token));
      g_variant_builder_add(&builder, "{sv}", "session_handle_token", g_variant_new_string(session_token));
      g_variant_builder_close(&builder);

      g_autoptr(GError) err = nullptr;
      g_autoptr(GVariant) reply = g_dbus_proxy_call_sync(proxy, "CreateSession", g_variant_builder_end(&builder), G_DBUS_CALL_FLAGS_NONE, dbus_timeout * 1000, nullptr, &err);

      if (err) {
        BOOST_LOG(error) << "[portalgrab] Could not create "sv << session_type << " session: "sv << err->message;
        return -1;
      }

      const gchar *request_path = nullptr;
      g_variant_get(reply, "(o)", &request_path);
      dbus_response_init(&response, loop, conn, request_path);

      g_autoptr(GVariant) create_response = dbus_response_wait(&response, dbus_timeout);

      if (!create_response) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " CreateSession: no response received"sv;
        return -1;
      }

      guint32 response_code;
      g_autoptr(GVariant) results = nullptr;
      g_variant_get(create_response, "(u@a{sv})", &response_code, &results);

      BOOST_LOG(debug) << "[portalgrab] " << session_type << " CreateSession response_code: "sv << response_code;

      if (response_code != 0) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " CreateSession failed with response code: "sv << response_code;
        return -1;
      }

      g_autoptr(GVariant) session_handle_v = g_variant_lookup_value(results, "session_handle", nullptr);
      if (!session_handle_v) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " CreateSession: session_handle not found in response"sv;
        return -1;
      }

      if (g_variant_is_of_type(session_handle_v, G_VARIANT_TYPE_VARIANT)) {
        g_autoptr(GVariant) inner = g_variant_get_variant(session_handle_v);
        *session_path_out = g_strdup(g_variant_get_string(inner, nullptr));
      } else {
        *session_path_out = g_strdup(g_variant_get_string(session_handle_v, nullptr));
      }

      BOOST_LOG(debug) << "[portalgrab] " << session_type << " CreateSession: got session handle: "sv << *session_path_out;
      // Save it for the destructor to use during cleanup
      this->session_handle = *session_path_out;
      return 0;
    }

    int select_remote_desktop_devices(GMainLoop *loop, const gchar *session_path) {
      dbus_response_t response {};
      g_autofree gchar *request_token = nullptr;
      create_request_path(conn, nullptr, &request_token);

      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("(oa{sv})"));
      g_variant_builder_add(&builder, "o", session_path);
      g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(request_token));
      g_variant_builder_add(&builder, "{sv}", "types", g_variant_new_uint32(TYPE_KEYBOARD | TYPE_POINTER | TYPE_TOUCHSCREEN));
      g_variant_builder_add(&builder, "{sv}", "persist_mode", g_variant_new_uint32(PERSIST_UNTIL_REVOKED));
      if (!restore_token_t::empty()) {
        g_variant_builder_add(&builder, "{sv}", "restore_token", g_variant_new_string(restore_token_t::get().c_str()));
      }
      g_variant_builder_close(&builder);

      g_autoptr(GError) err = nullptr;
      g_autoptr(GVariant) reply = g_dbus_proxy_call_sync(remote_desktop_proxy, "SelectDevices", g_variant_builder_end(&builder), G_DBUS_CALL_FLAGS_NONE, dbus_timeout * 1000, nullptr, &err);

      if (err) {
        BOOST_LOG(error) << "[portalgrab] Could not select devices: "sv << err->message;
        return -1;
      }

      const gchar *request_path = nullptr;
      g_variant_get(reply, "(o)", &request_path);
      dbus_response_init(&response, loop, conn, request_path);

      g_autoptr(GVariant) devices_response = dbus_response_wait(&response, dbus_timeout);

      if (!devices_response) {
        BOOST_LOG(error) << "[portalgrab] SelectDevices: no response received"sv;
        return -1;
      }

      guint32 response_code;
      g_variant_get(devices_response, "(u@a{sv})", &response_code, nullptr);
      BOOST_LOG(debug) << "[portalgrab] SelectDevices response_code: "sv << response_code;

      if (response_code != 0) {
        BOOST_LOG(error) << "[portalgrab] SelectDevices failed with response code: "sv << response_code;
        return -1;
      }

      return 0;
    }

    int select_screencast_sources(GMainLoop *loop, const gchar *session_path, bool persist) {
      dbus_response_t response {};
      g_autofree gchar *request_token = nullptr;
      create_request_path(conn, nullptr, &request_token);

      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("(oa{sv})"));
      g_variant_builder_add(&builder, "o", session_path);
      g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(request_token));
      // Virtual display mode: ask the portal for a new virtual screen instead of an existing monitor
      const bool virtual_mode = virtual_display::enabled();
      g_variant_builder_add(&builder, "{sv}", "types", g_variant_new_uint32(virtual_mode ? SOURCE_TYPE_VIRTUAL : SOURCE_TYPE_MONITOR));
      g_variant_builder_add(&builder, "{sv}", "cursor_mode", g_variant_new_uint32(CURSOR_MODE_EMBEDDED));
      g_variant_builder_add(&builder, "{sv}", "multiple", g_variant_new_boolean(virtual_mode ? FALSE : TRUE));
      if (persist) {
        g_variant_builder_add(&builder, "{sv}", "persist_mode", g_variant_new_uint32(PERSIST_UNTIL_REVOKED));
        if (!restore_token_t::empty()) {
          g_variant_builder_add(&builder, "{sv}", "restore_token", g_variant_new_string(restore_token_t::get().c_str()));
        }
      }
      g_variant_builder_close(&builder);

      g_autoptr(GError) err = nullptr;
      g_autoptr(GVariant) reply = g_dbus_proxy_call_sync(screencast_proxy, "SelectSources", g_variant_builder_end(&builder), G_DBUS_CALL_FLAGS_NONE, dbus_timeout * 1000, nullptr, &err);
      if (err) {
        BOOST_LOG(error) << "[portalgrab] Could not select sources: "sv << err->message;
        return -1;
      }

      const gchar *request_path = nullptr;
      g_variant_get(reply, "(o)", &request_path);
      dbus_response_init(&response, loop, conn, request_path);

      g_autoptr(GVariant) sources_response = dbus_response_wait(&response, dbus_timeout);

      if (!sources_response) {
        BOOST_LOG(error) << "[portalgrab] SelectSources: no response received"sv;
        return -1;
      }

      guint32 response_code;
      g_variant_get(sources_response, "(u@a{sv})", &response_code, nullptr);
      BOOST_LOG(debug) << "[portalgrab] SelectSources response_code: "sv << response_code;

      if (response_code != 0) {
        BOOST_LOG(error) << "[portalgrab] SelectSources failed with response code: "sv << response_code;
        return -1;
      }

      return 0;
    }

    int start_portal_session(GMainLoop *loop, const gchar *session_path, std::vector<pipewire_streaminfo_t> &out_pipewire_streams, bool use_screencast, bool allow_start_timeout) {
      GDBusProxy *proxy = use_screencast ? screencast_proxy : remote_desktop_proxy;
      const char *session_type = use_screencast ? "ScreenCast" : "RemoteDesktop";

      dbus_response_t response {};
      g_autofree gchar *request_token = nullptr;
      create_request_path(conn, nullptr, &request_token);

      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("(osa{sv})"));
      g_variant_builder_add(&builder, "o", session_path);
      g_variant_builder_add(&builder, "s", "");  // parent_window
      g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(request_token));
      g_variant_builder_close(&builder);

      g_autoptr(GError) err = nullptr;
      g_autoptr(GVariant) reply = g_dbus_proxy_call_sync(proxy, "Start", g_variant_builder_end(&builder), G_DBUS_CALL_FLAGS_NONE, dbus_timeout * 1000, nullptr, &err);
      if (err) {
        BOOST_LOG(error) << "[portalgrab] Could not start "sv << session_type << " session: "sv << err->message;
        return -1;
      }

      const gchar *request_path = nullptr;
      g_variant_get(reply, "(o)", &request_path);
      dbus_response_init(&response, loop, conn, request_path);

      g_autoptr(GVariant) start_response = dbus_response_wait(&response, (allow_start_timeout ? dbus_timeout : 0));

      if (!start_response) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " Start: no response received"sv;
        return -1;
      }

      guint32 response_code;
      g_autoptr(GVariant) dict = nullptr;
      g_autoptr(GVariant) streams = nullptr;
      g_variant_get(start_response, "(u@a{sv})", &response_code, &dict);

      BOOST_LOG(debug) << "[portalgrab] " << session_type << " Start response_code: "sv << response_code;

      if (response_code != 0) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " Start failed with response code: "sv << response_code;
        return -1;
      }

      streams = g_variant_lookup_value(dict, "streams", G_VARIANT_TYPE("a(ua{sv})"));
      if (!streams) {
        BOOST_LOG(error) << "[portalgrab] " << session_type << " Start: no streams in response"sv;
        return -1;
      }

      if (const gchar *new_token = nullptr; g_variant_lookup(dict, "restore_token", "s", &new_token) && new_token && new_token[0] != '\0' && restore_token_t::get() != new_token) {
        restore_token_t::set(new_token);
        restore_token_t::save();
      }

      GVariantIter iter;
      const auto wl_monitors = wl::monitors();
      uint32_t out_pipewire_node;
      g_autoptr(GVariant) value = nullptr;
      g_variant_iter_init(&iter, streams);
      while (g_variant_iter_next(&iter, "(u@a{sv})", &out_pipewire_node, &value)) {
        int out_width;
        int out_height;
        bool result = g_variant_lookup(value, "size", "(ii)", &out_width, &out_height, nullptr);
        if (!result) {
          BOOST_LOG(warning) << "[portalgrab] Ignoring stream without proper resolution on pipewire node "sv << out_pipewire_node;
          continue;
        }

        int out_pos_x;
        int out_pos_y;
        result = g_variant_lookup(value, "position", "(ii)", &out_pos_x, &out_pos_y, nullptr);
        if (!result) {
          BOOST_LOG(warning) << "[portalgrab] Falling back to position 0x0 for stream with resolution "sv << out_width << "x"sv << out_height << "on pipewire node "sv << out_pipewire_node;
          out_pos_x = 0;
          out_pos_y = 0;
        }

        uint64_t out_pipewire_object_serial;
        result = g_variant_lookup(value, "pipewire-serial", "t", &out_pipewire_object_serial);
        if (!result) {
          // If pipewire-serial was not present explicitly set to invalid value.
          out_pipewire_object_serial = SPA_ID_INVALID;
        }

        auto stream = pipewire_streaminfo_t {
          .pipewire_node = out_pipewire_node,
          .pipewire_object_serial = out_pipewire_object_serial,
          .width = out_width,
          .height = out_height,
          .pos_x = out_pos_x,
          .pos_y = out_pos_y,
        };

        // Try to match the stream to a monitor_name by position/resolution and update stream info
        for (const auto &monitor : wl_monitors) {
          if (monitor->viewport.offset_x == out_pos_x && monitor->viewport.offset_y == out_pos_y && monitor->viewport.logical_width == out_width && monitor->viewport.logical_height == out_height) {
            stream.monitor_name = monitor->name;
            break;
          }
        }

        out_pipewire_streams.emplace_back(stream);
      }

      // The portal call returns the streams sorted by out_pipewire_node which can shuffle displays around, so
      // we have to sort pipewire streams by position here to be consistent
      std::ranges::sort(out_pipewire_streams, [](const auto &a, const auto &b) {
        return a.pos_x < b.pos_x || a.pos_y < b.pos_y;
      });

      return 0;
    }

    int open_pipewire_remote(const gchar *session_path, int &fd) {
      g_autoptr(GUnixFDList) fd_list = nullptr;
      g_autoptr(GVariant) msg = g_variant_ref_sink(g_variant_new("(oa{sv})", session_path, nullptr));

      g_autoptr(GError) err = nullptr;
      g_autoptr(GVariant) reply = g_dbus_proxy_call_with_unix_fd_list_sync(screencast_proxy, "OpenPipeWireRemote", msg, G_DBUS_CALL_FLAGS_NONE, dbus_timeout * 1000, nullptr, &fd_list, nullptr, &err);
      if (err) {
        BOOST_LOG(error) << "[portalgrab] Could not open pipewire remote: "sv << err->message;
        return -1;
      }

      int fd_handle;
      g_variant_get(reply, "(h)", &fd_handle);
      fd = g_unix_fd_list_get(fd_list, fd_handle, nullptr);
      return 0;
    }

    static void on_response_received_cb([[maybe_unused]] GDBusConnection *connection, [[maybe_unused]] const gchar *sender_name, [[maybe_unused]] const gchar *object_path, [[maybe_unused]] const gchar *interface_name, [[maybe_unused]] const gchar *signal_name, GVariant *parameters, gpointer user_data) {
      auto *response = static_cast<dbus_response_t *>(user_data);
      response->response = g_variant_ref_sink(parameters);
      g_main_loop_quit(response->loop);
    }

    static gchar *get_sender_string(GDBusConnection *conn) {
      gchar *sender = g_strdup(g_dbus_connection_get_unique_name(conn) + 1);
      gchar *dot;
      while ((dot = strstr(sender, ".")) != nullptr) {
        *dot = '_';
      }
      return sender;
    }

    static void create_request_path(GDBusConnection *conn, gchar **out_path, gchar **out_token) {
      static uint32_t request_count = 0;

      request_count++;

      if (out_token) {
        *out_token = g_strdup_printf("Sunshine%u", request_count);
      }
      if (out_path) {
        g_autofree gchar *sender = get_sender_string(conn);
        *out_path = g_strdup(std::format("{}{}{}{}", REQUEST_PREFIX, sender, "/Sunshine", request_count).c_str());
      }
    }

    static void create_session_path(GDBusConnection *conn, gchar **out_path, gchar **out_token) {
      static uint32_t session_count = 0;

      session_count++;

      if (out_token) {
        *out_token = g_strdup_printf("Sunshine%u", session_count);
      }

      if (out_path) {
        g_autofree gchar *sender = get_sender_string(conn);
        *out_path = g_strdup(std::format("{}{}{}{}", SESSION_PREFIX, sender, "/Sunshine", session_count).c_str());
      }
    }

    /**
     * @brief Asynchronously close a pending Portal request.
     */
    static void close_request_async(GDBusConnection *conn, const std::string &request_path) {
      g_dbus_connection_call(
        conn,
        PORTAL_NAME,
        request_path.c_str(),
        REQUEST_IFACE,
        "Close",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        nullptr,
        nullptr
      );
    }

    static void dbus_response_init(struct dbus_response_t *response, GMainLoop *loop, GDBusConnection *conn, const char *request_path) {
      response->loop = loop;
      response->conn = conn;
      response->request_path = request_path;

      GMainContext *context = g_main_loop_get_context(loop);
      g_main_context_push_thread_default(context);

      response->subscription_id = g_dbus_connection_signal_subscribe(
        conn,
        PORTAL_NAME,
        REQUEST_IFACE,
        "Response",
        request_path,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_response_received_cb,
        response,
        nullptr
      );

      g_main_context_pop_thread_default(context);
    }

    /**
     * @brief Check for Sunshine shutdown and quit the Portal response loop if requested.
     *
     * @result True (continue) if shutdown event is not in progress.
     */
    static gboolean check_shutdown_cb(gpointer user_data) {
      if (auto shutdown_event = mail::man->event<bool>(mail::shutdown); shutdown_event->peek()) {
        g_main_loop_quit(static_cast<GMainLoop *>(user_data));
        return G_SOURCE_REMOVE;
      }
      return G_SOURCE_CONTINUE;
    }

    /**
     * @brief Check for DBus response with optional timeout guard.
     *
     * @param response DBus response.
     * @param timeout_seconds Timeout in seconds before quitting loop.
     * @return Variant containing the requested data.
     */
    static GVariant *dbus_response_wait(dbus_response_t *response, guint timeout_seconds = 0) {
      GSource *timeout_source = nullptr;

      if (timeout_seconds > 0) {
        timeout_source = g_timeout_source_new(timeout_seconds * 1000);
        g_source_set_callback(
          timeout_source,
          [](gpointer user_data) {
            g_main_loop_quit(static_cast<GMainLoop *>(user_data));
            return G_SOURCE_REMOVE;
          },
          response->loop,
          nullptr
        );
        g_source_attach(timeout_source, g_main_loop_get_context(response->loop));
      }

      constexpr guint shutdown_poll_interval_ms = 1000;
      GSource *shutdown_source = g_timeout_source_new(shutdown_poll_interval_ms);
      g_source_set_callback(shutdown_source, check_shutdown_cb, response->loop, nullptr);
      g_source_attach(shutdown_source, g_main_loop_get_context(response->loop));

      g_main_loop_run(response->loop);

      if (response->subscription_id != 0) {
        g_dbus_connection_signal_unsubscribe(
          response->conn,
          response->subscription_id
        );
      }
      response->subscription_id = 0;

      g_source_destroy(shutdown_source);
      g_source_unref(shutdown_source);

      if (timeout_source) {
        g_source_destroy(timeout_source);
        g_source_unref(timeout_source);
      }

      if (response->response) {
        return response->response;
      }

      BOOST_LOG(info) << "[portalgrab] Portal request cancelled, timed out, or shutdown requested"sv;
      close_request_async(response->conn, response->request_path);
      return nullptr;
    }
  };

  /**
   * @brief One virtual screen shared by all captures.
   *
   * Sunshine opens a capture for every encoder test at startup and again for the real
   * stream. Sharing one portal session means one virtual screen (and one KDE notification)
   * instead of a new one every time. It is closed a few seconds after the last capture ends.
   */
  struct shared_virtual_t {
    std::mutex mutex;  ///< Protects all fields.
    std::shared_ptr<dbus_t> dbus;  ///< Open portal session, if any.
    std::optional<virtual_display::session_t> vd;  ///< Current virtual screen state.
    int users = 0;  ///< Captures currently using the session.
    bool script_loaded = false;  ///< Whether the window-moving KWin script is active.
    uint64_t generation = 0;  ///< Changes on every acquire, to cancel pending releases.
  };

  inline shared_virtual_t &shared_virtual() {
    // Intentionally never destroyed: KDE closes the session itself when Sunshine exits
    static auto *state = new shared_virtual_t;
    return *state;
  }

  /**
   * @brief Get the shared session, creating it (and the virtual screen) if needed.
   */
  inline std::shared_ptr<dbus_t> acquire_virtual_session() {
    auto &sv = shared_virtual();
    std::lock_guard lock(sv.mutex);
    if (sv.dbus && !sv.dbus->is_session_closed()) {
      if (sv.dbus->reopen_pipewire_remote() < 0) {
        return nullptr;
      }
      BOOST_LOG(debug) << "[virtual_display] Reusing existing virtual screen"sv;
    } else {
      sv.dbus.reset();
      sv.vd.reset();
      auto dbus = std::make_shared<dbus_t>();
      if (dbus->init() < 0 || dbus->connect_to_portal(false) < 0 || dbus->pipewire_streams.empty()) {
        return nullptr;
      }
      sv.dbus = dbus;
    }
    sv.users++;
    sv.generation++;
    return sv.dbus;
  }

  /**
   * @brief Stop using the shared session; close it after a short grace period.
   */
  inline void release_virtual_session() {
    auto &sv = shared_virtual();
    uint64_t generation;
    {
      std::lock_guard lock(sv.mutex);
      if (--sv.users > 0) {
        return;
      }
      generation = sv.generation;
    }
    std::thread([generation]() {
      std::this_thread::sleep_for(8s);
      auto &sv = shared_virtual();
      std::lock_guard lock(sv.mutex);
      if (sv.users > 0 || sv.generation != generation) {
        return;  // Someone started using it again
      }
      if (sv.script_loaded) {
        virtual_display::unload_window_script();
        sv.script_loaded = false;
      }
      if (sv.vd) {
        virtual_display::restore(*sv.vd);
      }
      sv.vd.reset();
      sv.dbus.reset();  // Closes the portal session, KDE removes the virtual screen
      BOOST_LOG(info) << "[virtual_display] Virtual screen removed"sv;
    }).detach();
  }

  /**
   * @brief Portal screencast backend that negotiates PipeWire streams over DBus.
   */
  class portal_t: public pipewire::pipewire_display_t {
  public:
    ~portal_t() override {
      if (shared_dbus) {
        shared_dbus.reset();
        release_virtual_session();
      }
    }

    /**
     * @brief The portal session in use: the shared virtual one, or our own.
     */
    dbus_t &session() {
      return shared_dbus ? *shared_dbus : dbus;
    }

    /**
     * @brief Remember what the client asked for, then run the normal PipeWire setup.
     */
    int init(platf::mem_type_e hwdevice_type, const std::string &display_name, const ::video::config_t &config) {
      requested_width = config.width;
      requested_height = config.height;
      requested_fps = config.framerate;
      return pipewire::pipewire_display_t::init(hwdevice_type, display_name, config);
    }

    int configure_stream(const std::string &display_name, int &out_pipewire_fd, uint32_t &out_pipewire_node, uint64_t &out_pipewire_object_serial [[maybe_unused]]) override {
      // Connect DBus portal session
      if (dbus.init() < 0) {
        BOOST_LOG(error) << "[portalgrab] Failed to connect to dbus. portal_t setup failed.";
        return -1;
      }
      const bool virtual_mode = virtual_display::enabled();
      if (virtual_mode) {
        shared_dbus = acquire_virtual_session();
        if (!shared_dbus) {
          BOOST_LOG(error) << "[portalgrab] Failed to create virtual screen. portal_t setup failed.";
          return -1;
        }
      } else if (dbus.connect_to_portal(false) < 0) {
        BOOST_LOG(error) << "[portalgrab] Failed to connect to portal. portal_t setup failed.";
        return -1;
      }

      // Match display_name to a stream from the pipewire_streams vector
      bool use_fallback = true;
      pipewire_streaminfo_t stream;
      auto streams = session().pipewire_streams;
      if (streams.empty()) {
        BOOST_LOG(error) << "[portalgrab] No streams found on portal. portal_t setup failed.";
        return -1;
      }
      for (auto &stream_ : streams) {
        if (stream_.match_display_name(display_name)) {
          stream = stream_;
          use_fallback = false;
          break;
        }
      }
      // Fall back to first stream if we cannot match the given display_name to a stream in currently available streams.
      if (use_fallback) {
        BOOST_LOG(info) << "[portalgrab] Using first available stream as no matching stream was found for: '"sv << display_name << "'";
        stream = session().pipewire_streams.at(0);
      }

      // Virtual display mode: switch the new virtual screen to the client's resolution and refresh rate
      if (virtual_mode) {
        stream = session().pipewire_streams.at(0);
        auto vd = virtual_display::apply(requested_width, requested_height, requested_fps);
        if (vd) {
          stream.width = vd->width;
          stream.height = vd->height;
          std::lock_guard lock(shared_virtual().mutex);
          auto &shared = shared_virtual().vd;
          // Keep the original main screen across reuses, so it is restored correctly at the end
          if (shared && vd->previous_primary.empty()) {
            vd->previous_primary = shared->previous_primary;
          }
          shared = vd;
          if (!shared_virtual().script_loaded) {
            virtual_display::load_window_script();
            shared_virtual().script_loaded = true;
          }
        }
      }

      // Restore global maxframerate negotiation state
      pipewire.set_negotiate_maxframerate(negotiate_maxframerate.load());

      // Return values for pipewire init
      out_pipewire_fd = session().pipewire_fd;
      if (shared_dbus) {
        // PipeWire now owns this fd; make sure the shared session never closes it later
        shared_dbus->pipewire_fd = -1;
      }
      out_pipewire_node = stream.pipewire_node;
      out_pipewire_object_serial = stream.pipewire_object_serial;
      // Set/update basic stream parameters on display_t
      this->offset_x = stream.pos_x;
      this->offset_y = stream.pos_y;
      this->width = stream.width;
      this->height = stream.height;
      this->logical_width = 0;  // Explicitly mark for pipewire_display_t to try to figure this out.
      this->logical_height = 0;  // Explicitly Mark for pipewire_display_t to try to figure this out.
      // Flag successful setup
      return 0;
    }

    /**
     * @brief Check stream dead.
     *
     * @param out_status Out status.
     * @return True when the PipeWire stream can no longer produce frames.
     */
    bool check_stream_dead(platf::capture_e &out_status) override {
      // If the pipewire stream stopped due to closed portal session stop the capture with an error
      if (session().is_session_closed()) {
        BOOST_LOG(warning) << "[portalgrab] PipeWire stream stopped by closed portal session."sv;
        pipewire.frame_cv().notify_all();
        out_status = platf::capture_e::error;
        return true;  // Stop capture with error (due to out_status)
      }
      // Disable maxframerate negotiation if the stream died without having ever started (e.g. GNOME mutter does not support it)
      if (shared_state->previous_state != PW_STREAM_STATE_STREAMING && negotiate_maxframerate.load()) {
        BOOST_LOG(warning) << "[portalgrab] Negotiation failed, will retry without maxFramerate"sv;
        negotiate_maxframerate.store(false);
        pipewire.set_negotiate_maxframerate(false);
        out_status = platf::capture_e::reinit;
        return true;  // Stop capture with reinit (due to out_status)
      }
      return false;  // Return to default stream dead handling
    }

    // DBus portal connection
    dbus_t dbus;  ///< DBus connection used for portal screencast requests.

    // Virtual display state
    int requested_width = 1920;  ///< Width requested by the client.
    int requested_height = 1080;  ///< Height requested by the client.
    int requested_fps = 60;  ///< Refresh rate requested by the client.
    std::shared_ptr<dbus_t> shared_dbus;  ///< Shared virtual screen session, in virtual mode.

    // Class variable to store runtime state of maxFramerate negotiation
    static inline std::atomic<bool> negotiate_maxframerate {true};  ///< Whether portal negotiation should request the maximum frame rate.
  };
}  // namespace portal

namespace platf {
  /**
   * @brief Create a portal-based display capture backend.
   *
   * @param hwdevice_type Hardware device type requested for capture or encode.
   * @param display_name Display name.
   * @param config Configuration values to apply.
   * @return Display backend backed by xdg-desktop-portal and PipeWire, or nullptr.
   */
  std::shared_ptr<display_t> portal_display(mem_type_e hwdevice_type, const std::string &display_name, const video::config_t &config) {
    using enum platf::mem_type_e;
    if (!pipewire::pipewire_display_t::init_pipewire_and_check_hwdevice_type(hwdevice_type)) {
      BOOST_LOG(error) << "[portalgrab] Could not initialize pipewire-based display with the given hw device type."sv;
      return nullptr;
    }

    auto portal = std::make_shared<portal::portal_t>();
    if (portal->init(hwdevice_type, display_name, config)) {
      return nullptr;
    }

    return portal;
  }

  /**
   * @brief Enumerate capture targets available through xdg-desktop-portal.
   *
   * @param allow_start_timeout True if "Start" DBus call is allowed to time out.
   * @return Portal display names, or an empty list when portal discovery fails.
   */
  std::vector<std::string> portal_display_names(bool allow_start_timeout) {
    // Virtual display mode: there is nothing to enumerate. Opening a second portal session here
    // would make KDE hand out the same virtual screen twice, and closing it again would pull the
    // screen away from the running stream.
    if (portal::virtual_display::enabled()) {
      return {"virtual"};
    }

    std::vector<std::string> display_names;
    auto dbus = std::make_shared<portal::dbus_t>();

    if (dbus->init() < 0) {
      BOOST_LOG(warning) << "[portalgrab] Failed to connect to dbus. Cannot enumerate displays, returning empty list.";
      return {};
    }

    if (dbus->connect_to_portal(allow_start_timeout) < 0) {
      BOOST_LOG(warning) << "[portalgrab] Failed to connect to portal. Cannot enumerate displays, returning empty list.";
      return {};
    }

    for (auto stream_ : dbus->pipewire_streams) {
      BOOST_LOG(info) << "[portalgrab] Found stream for display id/name: '"sv << stream_.monitor_name << "' position: "sv << stream_.pos_x << "x"sv << stream_.pos_y << " resolution: "sv << stream_.width << "x"sv << stream_.height;
      display_names.emplace_back(stream_.to_display_name());
    }
    // Release the portal session as soon as possible to properly release related resources early.
    dbus.reset();

    // Return currently active display names
    return display_names;
  }
}  // namespace platf
