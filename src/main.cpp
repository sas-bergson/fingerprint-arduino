#include <chrono>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include "serialport.hpp"

/**
 * @file main.cpp
 * @brief Host-side command client for Arduino fingerprint services.
 *
 * This program supports two modes:
 * 1) Flash mode (`--flash`): compile/upload Arduino sketch using arduino-cli.
 * 2) Interactive service mode (default): send command codes to Arduino firmware
 *    and interpret status codes/responses for the fingerprint workflow.
 */

static constexpr int CMD_BOARD_STATUS = 10;      ///< Command: read board status
static constexpr int CMD_FP_STATUS = 20;         ///< Command: read fingerprint module status
static constexpr int CMD_READ_STORE_EXPORT = 30; ///< Command: read/store fingerprint and export template
static constexpr int CMD_CAPTURE_IMAGE = 40;     ///< Command: capture fingerprint image

static constexpr int STS_OK = 0;          ///< Status: success
static constexpr int STS_ERR = 1;         ///< Status: generic error
static constexpr int STS_INVALID_CMD = 2; ///< Status: unrecognised command code

static constexpr int SERIAL_LINE_TIMEOUT_MS = 8000; ///< Default timeout for reading one protocol line (ms)

/**
 * @brief Configuration for Arduino flash/upload mode.
 *
 * Populated from CLI arguments when `--flash` is supplied.
 * All fields have sensible defaults for a standard Arduino Uno setup.
 */
struct FlashConfig
{
  bool flashMode = false;                                ///< True when `--flash` was passed
  bool prepare = false;                                  ///< True when `--prepare` was passed (install core/libs)
  std::string cliPath;                                   ///< Explicit path to arduino-cli binary (empty = auto-resolve)
  std::string port = "/dev/ttyACM0";                     ///< Serial port for upload
  std::string fqbn = "arduino:avr:uno";                  ///< Fully qualified board name
  std::string sketchPath = "src/FingerPrint_Enroll.ino"; ///< Path to the Arduino sketch
};

/**
 * @brief Result of CLI argument parsing.
 */
enum class ParseStatus
{
  Ok,
  Help,
  Error
};

static int runCommand(const std::string &command);

/**
 * @brief Return local timestamp in `YYYY-MM-DD.HH:MM:SS` format.
 */
static std::string currentDateTime()
{
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  localtime_r(&now, &tstruct);
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
  return buf;
}

/**
 * @brief Return filename-safe timestamp.
 */
static std::string filenameTimestamp()
{
  std::string ts = currentDateTime();
  for (size_t i = 0; i < ts.size(); ++i)
  {
    if (ts[i] == ':')
    {
      ts[i] = '-';
    }
  }
  return ts;
}

/**
 * @brief Shell-escape argument for use with `std::system`.
 */
static std::string shellEscape(const std::string &value)
{
  std::string escaped = "'";
  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] == '\'')
    {
      escaped += "'\\''";
    }
    else
    {
      escaped.push_back(value[i]);
    }
  }
  escaped += "'";
  return escaped;
}

/**
 * @brief Resolve arduino-cli location.
 */
static std::string resolveArduinoCliPath(const std::string &explicitPath)
{
  if (!explicitPath.empty() && access(explicitPath.c_str(), X_OK) == 0)
  {
    return explicitPath;
  }

  const char *home = std::getenv("HOME");
  if (home != NULL)
  {
    const std::string localCli = std::string(home) + "/.local/bin/arduino-cli";
    if (access(localCli.c_str(), X_OK) == 0)
    {
      return localCli;
    }
  }

  return "arduino-cli";
}

/**
 * @brief Return true if string ends with suffix.
 */
static bool endsWith(const std::string &value, const std::string &suffix)
{
  return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Return directory portion of a path.
 */
static std::string dirnameOf(const std::string &path)
{
  const size_t pos = path.find_last_of('/');
  if (pos == std::string::npos)
  {
    return ".";
  }
  if (pos == 0)
  {
    return "/";
  }
  return path.substr(0, pos);
}

/**
 * @brief Return basename portion of a path.
 */
static std::string basenameOf(const std::string &path)
{
  const size_t pos = path.find_last_of('/');
  if (pos == std::string::npos)
  {
    return path;
  }
  return path.substr(pos + 1);
}

/**
 * @brief Return filename stem without extension.
 */
static std::string stemOf(const std::string &path)
{
  const std::string base = basenameOf(path);
  const size_t dot = base.find_last_of('.');
  if (dot == std::string::npos)
  {
    return base;
  }
  return base.substr(0, dot);
}

/**
 * @brief Ensure export directory structure exists for fingerprint data.
 *
 * Creates `exports/templates/` and `exports/images/` subdirectories if they don't exist.
 * Uses `mkdir -p` via system call for robustness.
 *
 * @return true if directories exist or were created successfully, false on error.
 */
static bool ensureExportDirectories()
{
  const std::string exportRoot = "exports";
  const std::string templatesDir = exportRoot + "/templates";
  const std::string imagesDir = exportRoot + "/images";

  // Create both directories with mkdir -p (no error if already exist)
  const std::string mkdirCmd = "mkdir -p " + shellEscape(templatesDir) + " " + shellEscape(imagesDir);
  if (std::system(mkdirCmd.c_str()) != 0)
  {
    std::cerr << "Error: could not create export directories." << std::endl;
    return false;
  }
  return true;
}

/**
 * @brief Stage a sketch file into an Arduino-compatible directory layout.
 *
 * Arduino CLI expects the sketch directory name to match the main `.ino`
 * filename. This helper copies the sketch into `build/arduino-cli-sketches/<stem>/<stem>.ino`.
 *
 * @param sketchPath Original sketch file path.
 * @return Path to the staged sketch directory, or empty string on failure.
 */
static std::string prepareSketchDirectory(const std::string &sketchPath)
{
  const std::string sketchStem = stemOf(sketchPath);
  const std::string stagingRoot = "build/arduino-cli-sketches";
  const std::string stagingDir = stagingRoot + "/" + sketchStem;
  const std::string stagedSketchPath = stagingDir + "/" + sketchStem + ".ino";

  const std::string mkdirCommand = "mkdir -p " + shellEscape(stagingDir);
  if (runCommand(mkdirCommand) != 0)
  {
    return "";
  }

  const std::string copyCommand = "cp " + shellEscape(sketchPath) + " " + shellEscape(stagedSketchPath);
  if (runCommand(copyCommand) != 0)
  {
    return "";
  }

  return stagingDir;
}

/**
 * @brief Execute shell command and return its exit status.
 */
static int runCommand(const std::string &command)
{
  std::cout << "Executing: " << command << std::endl;
  return std::system(command.c_str());
}

/**
 * @brief Print command-line help.
 */
static void printUsage(const char *programName)
{
  std::cout
      << "Usage:\n"
      << "  " << programName << "                           # Start service menu mode\n"
      << "  " << programName << " --flash [options]         # Compile and upload Arduino sketch\n\n"
      << "Flash options:\n"
      << "  --port <serial-port>    Default: /dev/ttyACM0\n"
      << "  --board <fqbn>          Default: arduino:avr:uno\n"
      << "  --sketch <path>         Default: src/FingerPrint_Enroll.ino\n"
      << "  --cli <path>            Path to arduino-cli binary\n"
      << "  --prepare               Install required board core and library first\n"
      << "  --help                  Show this help\n";
}

/**
 * @brief Parse CLI arguments.
 */
static ParseStatus parseArguments(int argc, char **argv, FlashConfig &config)
{
  for (int i = 1; i < argc; ++i)
  {
    const std::string arg(argv[i]);
    if (arg == "--help" || arg == "-h")
    {
      return ParseStatus::Help;
    }
    if (arg == "--flash")
    {
      config.flashMode = true;
      continue;
    }
    if (arg == "--prepare")
    {
      config.prepare = true;
      continue;
    }
    if (arg == "--port" && i + 1 < argc)
    {
      config.port = argv[++i];
      continue;
    }
    if (arg == "--board" && i + 1 < argc)
    {
      config.fqbn = argv[++i];
      continue;
    }
    if (arg == "--sketch" && i + 1 < argc)
    {
      config.sketchPath = argv[++i];
      continue;
    }
    if (arg == "--cli" && i + 1 < argc)
    {
      config.cliPath = argv[++i];
      continue;
    }

    std::cerr << "Unknown argument: " << arg << std::endl;
    return ParseStatus::Error;
  }

  return ParseStatus::Ok;
}

/**
 * @brief Flash Arduino firmware using arduino-cli.
 */
static int flashArduino(const FlashConfig &config)
{
  std::ifstream sketchFile(config.sketchPath.c_str());
  if (!sketchFile)
  {
    std::cerr << "Error: sketch file not found: " << config.sketchPath << std::endl;
    return EXIT_FAILURE;
  }

  const std::string cli = resolveArduinoCliPath(config.cliPath);
  const std::string cliEscaped = shellEscape(cli);
  const std::string fqbnEscaped = shellEscape(config.fqbn);
  const std::string portEscaped = shellEscape(config.port);
  std::string compileTarget = config.sketchPath;

  if (endsWith(config.sketchPath, ".ino"))
  {
    compileTarget = prepareSketchDirectory(config.sketchPath);
    if (compileTarget.empty())
    {
      std::cerr << "Error: failed to prepare staged sketch directory for arduino-cli." << std::endl;
      return EXIT_FAILURE;
    }
  }

  const std::string sketchEscaped = shellEscape(compileTarget);

  if (runCommand(cliEscaped + " version") != 0)
  {
    std::cerr << "Error: arduino-cli is not available. Use --cli <path> or install it first." << std::endl;
    return EXIT_FAILURE;
  }

  if (config.prepare)
  {
    if (runCommand(cliEscaped + " core update-index") != 0)
    {
      std::cerr << "Error: failed to update core index." << std::endl;
      return EXIT_FAILURE;
    }
    if (runCommand(cliEscaped + " core install arduino:avr") != 0)
    {
      std::cerr << "Error: failed to install board core arduino:avr." << std::endl;
      return EXIT_FAILURE;
    }
    if (runCommand(cliEscaped + " lib update-index") != 0)
    {
      std::cerr << "Error: failed to update library index." << std::endl;
      return EXIT_FAILURE;
    }
    if (runCommand(cliEscaped + " lib install \"Adafruit Fingerprint Sensor Library\"") != 0)
    {
      std::cerr << "Error: failed to install required library: Adafruit Fingerprint Sensor Library." << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (runCommand(cliEscaped + " compile --fqbn " + fqbnEscaped + " " + sketchEscaped) != 0)
  {
    std::cerr << "Error: compile failed. Ensure board core and libraries are installed." << std::endl;
    return EXIT_FAILURE;
  }

  if (runCommand(cliEscaped + " upload -p " + portEscaped + " --fqbn " + fqbnEscaped + " " + sketchEscaped) != 0)
  {
    std::cerr << "Error: upload failed. Check USB port, board type, and permissions." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Flash completed successfully." << std::endl;
  return EXIT_SUCCESS;
}

/**
 * @brief Split input string by one delimiter.
 */
static std::vector<std::string> split(const std::string &value, char delim)
{
  std::vector<std::string> out;
  std::stringstream ss(value);
  std::string item;
  while (std::getline(ss, item, delim))
  {
    out.push_back(item);
  }
  return out;
}

/**
 * @brief Format duration as `MMmSSs`.
 */
static std::string formatDuration(std::chrono::steady_clock::duration d)
{
  const long long totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(d).count();
  const long long minutes = totalSeconds / 60LL;
  const long long seconds = totalSeconds % 60LL;

  std::ostringstream os;
  os << std::setw(2) << std::setfill('0') << minutes << "m"
     << std::setw(2) << std::setfill('0') << seconds << "s";
  return os.str();
}

/**
 * @brief Print one CLI-style transfer progress line in place.
 */
static void printTransferProgress(const std::string &label,
                                  size_t bytesReceived,
                                  size_t bytesTarget,
                                  int packets,
                                  std::chrono::steady_clock::time_point start)
{
  static const int kBarWidth = 40;
  int filled = 0;
  int percent = 0;

  if (bytesTarget > 0)
  {
    const double ratio = static_cast<double>(bytesReceived) / static_cast<double>(bytesTarget);
    const double clamped = (ratio < 0.0) ? 0.0 : (ratio > 1.0 ? 1.0 : ratio);
    filled = static_cast<int>(clamped * kBarWidth);
    percent = static_cast<int>(clamped * 100.0);
  }

  std::ostringstream bar;
  for (int i = 0; i < kBarWidth; ++i)
  {
    if (i < filled)
    {
      bar << '=';
    }
    else if (i == filled && filled < kBarWidth)
    {
      bar << '>';
    }
    else
    {
      bar << '-';
    }
  }

  const std::string elapsed = formatDuration(std::chrono::steady_clock::now() - start);
  std::cout << "\r\033[2K" << label << " " << bytesReceived << " B / " << bytesTarget
            << " B [" << bar.str() << "] "
            << std::setw(3) << percent << "% "
            << elapsed << " packets=" << packets << std::flush;
}

/**
 * @brief Print final transfer summary.
 */
static void printTransferSummary(const std::string &label,
                                 int packets,
                                 size_t bytesReceived,
                                 std::chrono::steady_clock::time_point start)
{
  const std::string elapsed = formatDuration(std::chrono::steady_clock::now() - start);
  std::cout << "\n"
            << label << " summary: packets=" << packets
            << ", bytes=" << bytesReceived
            << ", elapsed=" << elapsed << std::endl;
}

/**
 * @brief Background progress renderer for long-running transfers.
 */
class AsyncProgressDisplay
{
public:
  /**
   * @brief Construct a progress display.
   * @param label     Text label shown before the progress bar.
   * @param initialTarget  Expected total byte count (used to size the bar).
   * @param start     Timestamp recorded when the transfer began.
   */
  AsyncProgressDisplay(const std::string &label,
                       size_t initialTarget,
                       std::chrono::steady_clock::time_point start)
      : label_(label),
        start_(start),
        bytesReceived_(0),
        bytesTarget_(initialTarget),
        packets_(0),
        running_(false)
  {
  }

  ~AsyncProgressDisplay()
  {
    stop();
  }

  AsyncProgressDisplay(const AsyncProgressDisplay &) = delete;
  AsyncProgressDisplay &operator=(const AsyncProgressDisplay &) = delete;

  /**
   * @brief Update transfer statistics rendered by the background thread.
   * @param bytesReceived  Bytes received so far.
   * @param bytesTarget    Updated expected total bytes.
   * @param packets        Packet count received so far.
   */
  void update(size_t bytesReceived, size_t bytesTarget, int packets)
  {
    bytesReceived_.store(bytesReceived);
    bytesTarget_.store(bytesTarget);
    packets_.store(packets);
  }

  /** @brief Start the background rendering thread. No-op if already running. */
  void start()
  {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
    {
      return;
    }

    worker_ = std::thread([this]()
                          {
                            while (running_.load())
                            {
                              printTransferProgress(label_,
                                                    bytesReceived_.load(),
                                                    bytesTarget_.load(),
                                                    packets_.load(),
                                                    start_);
                              std::this_thread::sleep_for(std::chrono::milliseconds(150));
                            }

                            printTransferProgress(label_,
                                                  bytesReceived_.load(),
                                                  bytesTarget_.load(),
                                                  packets_.load(),
                                                  start_); });
  }

  /** @brief Stop the background thread and block until it exits. */
  void stop()
  {
    const bool wasRunning = running_.exchange(false);
    if ((wasRunning || worker_.joinable()) && worker_.joinable())
    {
      worker_.join();
    }
  }

private:
  std::string label_;                           ///< Label text shown before the progress bar
  std::chrono::steady_clock::time_point start_; ///< Timestamp when the transfer began
  std::atomic<size_t> bytesReceived_;           ///< Bytes received so far
  std::atomic<size_t> bytesTarget_;             ///< Expected total byte count
  std::atomic<int> packets_;                    ///< Packet count received so far
  std::atomic<bool> running_;                   ///< True while the worker thread is active
  std::thread worker_;                          ///< Background rendering thread
};

/**
 * @brief Parse target byte count from key-value response payload.
 */
static size_t parseTargetBytesFromPayload(const std::string &payload,
                                          size_t fallbackBytes,
                                          bool imageMode)
{
  size_t targetBytes = fallbackBytes;
  int width = 0;
  int height = 0;
  std::string format;

  const std::vector<std::string> attrs = split(payload, ';');
  for (size_t i = 0; i < attrs.size(); ++i)
  {
    const std::vector<std::string> kv = split(attrs[i], '=');
    if (kv.size() != 2)
    {
      continue;
    }

    if (kv[0] == "bytes")
    {
      const long parsed = std::atol(kv[1].c_str());
      if (parsed > 0)
      {
        targetBytes = static_cast<size_t>(parsed);
      }
    }
    else if (kv[0] == "width")
    {
      width = std::atoi(kv[1].c_str());
    }
    else if (kv[0] == "height")
    {
      height = std::atoi(kv[1].c_str());
    }
    else if (kv[0] == "format")
    {
      format = kv[1];
    }
  }

  if (!imageMode)
  {
    return targetBytes;
  }

  if (width > 0 && height > 0)
  {
    const size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (format == "gray4_packed")
    {
      targetBytes = pixels / 2U;
    }
    else if (format == "gray8")
    {
      targetBytes = pixels;
    }
  }

  return targetBytes;
}

/**
 * @brief Read one newline-delimited serial line with timeout.
 */
static bool readSerialLine(SerialPort &arduino, std::string &line, int timeoutMs)
{
  line.clear();
  const auto start = std::chrono::steady_clock::now();
  char ch = '\0';

  while (true)
  {
    const int n = arduino.readSerialPort(&ch, 1);
    if (n > 0)
    {
      if (ch == '\n')
      {
        if (!line.empty() && line.back() == '\r')
        {
          line.pop_back();
        }
        return true;
      }
      line.push_back(ch);
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (elapsed >= timeoutMs)
    {
      return false;
    }
  }
}

/**
 * @brief Read one serial byte with timeout.
 */
static bool readSerialByteWithTimeout(SerialPort &arduino, char &out, int timeoutMs)
{
  const auto start = std::chrono::steady_clock::now();
  while (true)
  {
    const int n = arduino.readSerialPort(&out, 1);
    if (n > 0)
    {
      return true;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (elapsed >= timeoutMs)
    {
      return false;
    }
  }
}

/**
 * @brief Read an exact number of bytes from serial with timeout.
 */
static bool readSerialBytesExact(SerialPort &arduino, std::vector<uint8_t> &out, size_t count, int timeoutMs)
{
  out.clear();
  out.reserve(count);
  const auto start = std::chrono::steady_clock::now();

  while (out.size() < count)
  {
    char buffer[64];
    const size_t remaining = count - out.size();
    const unsigned int requestSize = static_cast<unsigned int>(remaining > sizeof(buffer) ? sizeof(buffer) : remaining);
    const int n = arduino.readSerialPort(buffer, requestSize);
    if (n > 0)
    {
      for (int i = 0; i < n; ++i)
      {
        out.push_back(static_cast<uint8_t>(buffer[i]));
      }
      continue;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (elapsed >= timeoutMs)
    {
      return false;
    }
  }

  return true;
}

/**
 * @brief Best-effort drain of pending serial input for a short idle window.
 */
static void drainSerialInput(SerialPort &arduino, int idleTimeoutMs)
{
  const auto start = std::chrono::steady_clock::now();
  char ch = '\0';

  while (true)
  {
    const int n = arduino.readSerialPort(&ch, 1);
    if (n > 0)
    {
      continue;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (elapsed >= idleTimeoutMs)
    {
      break;
    }
  }
}

/**
 * @brief Consume one IMB frame delimiter (LF or CRLF).
 */
static bool consumeImageFrameDelimiter(SerialPort &arduino, int timeoutMs)
{
  const auto start = std::chrono::steady_clock::now();
  char delim = '\0';

  while (true)
  {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (elapsed >= timeoutMs)
    {
      return false;
    }

    if (!readSerialByteWithTimeout(arduino, delim, timeoutMs - static_cast<int>(elapsed)))
    {
      return false;
    }

    if (delim == '\n')
    {
      return true;
    }

    // Ignore CR/stray bytes until a newline delimiter is found.
  }
}

/**
 * @brief Drain startup events right after opening the serial port.
 *
 * Opening a serial connection can reset an Arduino Uno. During this short boot
 * window, commands may be dropped. We wait briefly for protocol event lines and
 * print them so the first menu command is less likely to timeout.
 */
static void waitForStartupEvents(SerialPort &arduino)
{
  for (int i = 0; i < 3; ++i)
  {
    std::string line;
    if (!readSerialLine(arduino, line, 1200))
    {
      break;
    }

    if (line.rfind("EVT|", 0) == 0)
    {
      std::cout << "[event] " << line << std::endl;
      continue;
    }

    // Non-event line: ignore once during startup sync.
  }
}

/**
 * @brief Send one command line to Arduino protocol endpoint.
 */
static bool sendCommand(SerialPort &arduino, int commandCode, int arg)
{
  std::ostringstream os;
  os << "CMD|" << commandCode << "|" << arg << "\n";
  const std::string cmd = os.str();
  return arduino.writeSerialPort(cmd.c_str(), static_cast<unsigned int>(cmd.size())) > 0;
}

/**
 * @brief Parse protocol status line with shape `RSP|cmd|status|payload`.
 */
static bool parseResponseLine(const std::string &line, int &cmd, int &status, std::string &payload)
{
  const std::vector<std::string> parts = split(line, '|');
  if (parts.size() < 4 || parts[0] != "RSP")
  {
    return false;
  }

  cmd = std::atoi(parts[1].c_str());
  status = std::atoi(parts[2].c_str());
  payload = parts[3];
  return true;
}

/**
 * @brief Parse template packet line with shape `TPL|idx|hex-bytes`.
 */
static bool parseTemplateLine(const std::string &line, std::string &hexPayload)
{
  const std::vector<std::string> parts = split(line, '|');
  if (parts.size() < 3 || parts[0] != "TPL")
  {
    return false;
  }
  hexPayload = parts[2];
  return true;
}

/**
 * @brief Parse image packet line with shape `IMG|idx|hex-bytes`.
 */
static bool parseImageLine(const std::string &line, std::string &hexPayload)
{
  const std::vector<std::string> parts = split(line, '|');
  if (parts.size() < 3 || parts[0] != "IMG")
  {
    return false;
  }
  hexPayload = parts[2];
  return true;
}

/**
 * @brief Parse binary image header line with shape `IMB|idx|payload_len`.
 */
static bool parseImageBinaryHeader(const std::string &line, int &payloadLen)
{
  const std::vector<std::string> parts = split(line, '|');
  if (parts.size() < 3 || parts[0] != "IMB")
  {
    return false;
  }

  payloadLen = std::atoi(parts[2].c_str());
  return payloadLen > 0 && payloadLen <= 256;
}

/**
 * @brief Convert whitespace-separated hex string into raw bytes.
 */
static std::vector<uint8_t> parseHexBytes(const std::string &hexLine)
{
  std::vector<uint8_t> bytes;
  bool hasWhitespace = false;
  for (size_t i = 0; i < hexLine.size(); ++i)
  {
    if (hexLine[i] == ' ' || hexLine[i] == '\t' || hexLine[i] == '\r' || hexLine[i] == '\n')
    {
      hasWhitespace = true;
      break;
    }
  }

  if (hasWhitespace)
  {
    std::stringstream ss(hexLine);
    std::string token;
    while (ss >> token)
    {
      unsigned int value = 0;
      std::stringstream hs;
      hs << std::hex << token;
      hs >> value;
      bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
    }
    return bytes;
  }

  if (hexLine.size() % 2 != 0)
  {
    return bytes;
  }

  for (size_t i = 0; i + 1 < hexLine.size(); i += 2)
  {
    unsigned int value = 0;
    std::stringstream hs;
    hs << std::hex << hexLine.substr(i, 2);
    hs >> value;
    bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
  }
  return bytes;
}

/**
 * @brief Expand packed 4-bit grayscale bytes (2 pixels/byte) to 8-bit grayscale.
 */
static std::vector<uint8_t> expandGray4PackedToGray8(const std::vector<uint8_t> &packedBytes, int width, int height)
{
  const size_t expectedPixels = static_cast<size_t>(width) * static_cast<size_t>(height);
  std::vector<uint8_t> out;
  out.reserve(expectedPixels);

  for (size_t i = 0; i < packedBytes.size() && out.size() < expectedPixels; ++i)
  {
    const uint8_t byte = packedBytes[i];
    const uint8_t high = static_cast<uint8_t>((byte >> 4) & 0x0FU);
    const uint8_t low = static_cast<uint8_t>(byte & 0x0FU);
    out.push_back(static_cast<uint8_t>(high * 17U));
    if (out.size() < expectedPixels)
    {
      out.push_back(static_cast<uint8_t>(low * 17U));
    }
  }

  return out;
}

/**
 * @brief Persist fingerprint template bytes to `.txt` and `.bin` output files.
 *
 * Exports are saved to `exports/templates/` subdirectory with timestamp and ID.
 * Creates hex-encoded `.txt` and binary `.bin` versions for flexibility.
 *
 * @param bytes Template data bytes.
 * @param fingerId Fingerprint ID for filename identification.
 */
static void persistFingerprintTemplate(const std::vector<uint8_t> &bytes, int fingerId)
{
  if (!ensureExportDirectories())
  {
    return;
  }

  const std::string baseName = "exports/templates/" + filenameTimestamp() + "_id" + std::to_string(fingerId);
  std::ofstream textFile(baseName + ".txt", std::ios::out | std::ios::trunc);
  std::ofstream binaryFile(baseName + ".bin", std::ios::out | std::ios::binary | std::ios::trunc);

  if (!textFile || !binaryFile)
  {
    std::cerr << "Error: could not create local fingerprint files." << std::endl;
    return;
  }

  for (size_t i = 0; i < bytes.size(); ++i)
  {
    textFile << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(bytes[i]);
    if (i + 1 != bytes.size())
    {
      textFile << ' ';
    }
  }
  textFile << std::endl;

  if (!bytes.empty())
  {
    binaryFile.write(reinterpret_cast<const char *>(&bytes[0]), static_cast<std::streamsize>(bytes.size()));
  }

  std::cout << "Local fingerprint template persisted: " << baseName
            << " (" << bytes.size() << " bytes)" << std::endl;
}

/**
 * @brief Persist raw grayscale image to local uncompressed 8-bit BMP file.
 *
 * Exports are saved to `exports/images/` subdirectory in standard Windows BMP format
 * with 8-bit grayscale palette. Row-major layout with proper padding.
 *
 * @param bytes Raw grayscale image data (8 bits per pixel).
 * @param width Image width in pixels (typically 256).
 * @param height Image height in pixels (typically 288).
 */
static void persistFingerprintImageBmp(const std::vector<uint8_t> &bytes, int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    std::cerr << "Error: invalid image dimensions for BMP export." << std::endl;
    return;
  }

  const size_t expectedPixels = static_cast<size_t>(width) * static_cast<size_t>(height);
  if (bytes.size() < expectedPixels)
  {
    std::cerr << "Error: insufficient image payload for BMP export. Got " << bytes.size()
              << " bytes, expected at least " << expectedPixels << "." << std::endl;
    return;
  }

  if (!ensureExportDirectories())
  {
    return;
  }

  const std::string fileName = "exports/images/fingerprint_" + filenameTimestamp() + ".bmp";
  std::ofstream out(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out)
  {
    std::cerr << "Error: could not create image file: " << fileName << std::endl;
    return;
  }

  const uint32_t rowStride = static_cast<uint32_t>((width + 3) & ~3);
  const uint32_t pixelDataSize = rowStride * static_cast<uint32_t>(height);
  const uint32_t paletteSize = 256U * 4U;
  const uint32_t fileHeaderSize = 14U;
  const uint32_t dibHeaderSize = 40U;
  const uint32_t pixelDataOffset = fileHeaderSize + dibHeaderSize + paletteSize;
  const uint32_t fileSize = pixelDataOffset + pixelDataSize;

  auto writeLe16 = [&out](uint16_t value)
  {
    const char b[2] = {
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8) & 0xFFU)};
    out.write(b, 2);
  };

  auto writeLe32 = [&out](uint32_t value)
  {
    const char b[4] = {
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8) & 0xFFU),
        static_cast<char>((value >> 16) & 0xFFU),
        static_cast<char>((value >> 24) & 0xFFU)};
    out.write(b, 4);
  };

  // BITMAPFILEHEADER (14 bytes)
  out.write("BM", 2);
  writeLe32(fileSize);
  writeLe16(0);
  writeLe16(0);
  writeLe32(pixelDataOffset);

  // BITMAPINFOHEADER (40 bytes)
  writeLe32(dibHeaderSize);
  writeLe32(static_cast<uint32_t>(width));
  writeLe32(static_cast<uint32_t>(height));
  writeLe16(1);
  writeLe16(8);
  writeLe32(0);
  writeLe32(pixelDataSize);
  writeLe32(5000);
  writeLe32(5000);
  writeLe32(256);
  writeLe32(0);

  // 8-bit grayscale palette (BGRA entries)
  for (int i = 0; i < 256; ++i)
  {
    const char entry[4] = {
        static_cast<char>(i),
        static_cast<char>(i),
        static_cast<char>(i),
        static_cast<char>(0)};
    out.write(entry, 4);
  }

  // BMP stores rows bottom-up for positive height.
  const uint32_t padding = rowStride - static_cast<uint32_t>(width);
  const char pad[3] = {0, 0, 0};
  for (int y = height - 1; y >= 0; --y)
  {
    const size_t rowStart = static_cast<size_t>(y) * static_cast<size_t>(width);
    out.write(reinterpret_cast<const char *>(&bytes[rowStart]), static_cast<std::streamsize>(width));
    if (padding > 0)
    {
      out.write(pad, static_cast<std::streamsize>(padding));
    }
  }

  std::cout << "Local fingerprint image persisted: " << fileName
            << " (" << expectedPixels << " pixels, " << bytes.size() << " raw bytes)" << std::endl;
}

/**
 * @brief Request one command and wait for one status response.
 */
static bool requestSimpleStatus(SerialPort &arduino, int commandCode)
{
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    if (!sendCommand(arduino, commandCode, 0))
    {
      std::cerr << "Serial write failed." << std::endl;
      return false;
    }

    std::string line;
    if (!readSerialLine(arduino, line, SERIAL_LINE_TIMEOUT_MS))
    {
      if (attempt == 0)
      {
        std::cerr << "Timeout waiting for response, retrying command..." << std::endl;
        continue;
      }
      std::cerr << "Timeout waiting for response." << std::endl;
      return false;
    }

    while (true)
    {
      if (line.rfind("EVT|", 0) == 0)
      {
        std::cout << "[event] " << line << std::endl;
      }
      else
      {
        int cmd = 0;
        int status = STS_ERR;
        std::string payload;
        if (parseResponseLine(line, cmd, status, payload) && cmd == commandCode)
        {
          std::cout << "Command " << commandCode << " status=" << status << std::endl;
          std::cout << "Data: " << payload << std::endl;
          return status == STS_OK;
        }
      }

      if (!readSerialLine(arduino, line, SERIAL_LINE_TIMEOUT_MS))
      {
        break;
      }
    }
  }

  std::cerr << "No valid response received." << std::endl;
  return false;
}

/**
 * @brief Trigger fingerprint read/store/export flow and persist returned template.
 */
static bool requestFingerprintReadStore(SerialPort &arduino)
{
  std::cout << "Enter fingerprint ID to store on module (1-127): ";
  std::string idInput;
  if (!std::getline(std::cin, idInput))
  {
    return false;
  }

  const int fingerId = std::atoi(idInput.c_str());
  if (fingerId < 1 || fingerId > 127)
  {
    std::cerr << "Invalid fingerprint ID." << std::endl;
    return false;
  }

  drainSerialInput(arduino, 30);

  if (!sendCommand(arduino, CMD_READ_STORE_EXPORT, fingerId))
  {
    std::cerr << "Serial write failed." << std::endl;
    return false;
  }

  std::vector<uint8_t> allBytes;
  int packetCount = 0;
  bool templateStreamingStarted = false;
  const auto transferStart = std::chrono::steady_clock::now();
  // Typical template export on this module is 640 bytes (20 packets * 32 bytes).
  static const size_t TEMPLATE_TARGET_BYTES = 640U;
  static const int TEMPLATE_FLOW_TIMEOUT_MS = 120000;
  size_t targetTemplateBytes = TEMPLATE_TARGET_BYTES;

  while (true)
  {
    std::string line;
    if (!readSerialLine(arduino, line, TEMPLATE_FLOW_TIMEOUT_MS))
    {
      std::cerr << "Timeout waiting for fingerprint flow response." << std::endl;
      return false;
    }

    if (line.rfind("EVT|", 0) == 0)
    {
      std::cout << "[event] " << line << std::endl;
      if (line.find("|30|200|") != std::string::npos)
      {
        templateStreamingStarted = true;
      }
      continue;
    }

    std::string hexPayload;
    if (templateStreamingStarted && parseTemplateLine(line, hexPayload))
    {
      const std::vector<uint8_t> bytes = parseHexBytes(hexPayload);
      allBytes.insert(allBytes.end(), bytes.begin(), bytes.end());
      ++packetCount;
      printTransferProgress("Downloading template:", allBytes.size(), targetTemplateBytes, packetCount, transferStart);
      continue;
    }

    int cmd = 0;
    int status = STS_ERR;
    std::string payload;
    if (!parseResponseLine(line, cmd, status, payload))
    {
      continue;
    }

    if (cmd != CMD_READ_STORE_EXPORT)
    {
      continue;
    }

    if (packetCount > 0)
    {
      std::cout << std::endl;
    }
    std::cout << "Command " << CMD_READ_STORE_EXPORT << " status=" << status << std::endl;
    std::cout << "Data: " << payload << std::endl;
    targetTemplateBytes = parseTargetBytesFromPayload(payload, TEMPLATE_TARGET_BYTES, false);
    printTransferSummary("Template transfer", packetCount, allBytes.size(), transferStart);
    if (status == STS_OK)
    {
      persistFingerprintTemplate(allBytes, fingerId);
      return true;
    }
    return false;
  }
}

/**
 * @brief Trigger image capture flow and persist returned image as BMP.
 */
static bool requestFingerprintImageCapture(SerialPort &arduino)
{
  static const int IMAGE_CAPTURE_LINE_TIMEOUT_MS = 120000;
  static const size_t IMAGE_TARGET_PACKED_BYTES = (256U * 288U) / 2U;

  drainSerialInput(arduino, 80);

  if (!sendCommand(arduino, CMD_CAPTURE_IMAGE, 0))
  {
    std::cerr << "Serial write failed." << std::endl;
    return false;
  }

  std::vector<uint8_t> imageBytes;
  int imageWidth = 256;
  int imageHeight = 288;
  int fullImageHeight = imageHeight;
  std::string imageFormat = "gray8";
  int packetCount = 0;
  bool imageStreamingStarted = false;
  int delimiterErrorCount = 0;
  const auto transferStart = std::chrono::steady_clock::now();
  size_t targetImageBytes = IMAGE_TARGET_PACKED_BYTES;

  while (true)
  {
    std::string line;
    if (!readSerialLine(arduino, line, IMAGE_CAPTURE_LINE_TIMEOUT_MS))
    {
      std::cerr << "Timeout waiting for image capture response." << std::endl;
      return false;
    }

    if (line.rfind("EVT|", 0) == 0)
    {
      std::cout << "[event] " << line << std::endl;
      if (line.find("|40|200|") != std::string::npos)
      {
        imageStreamingStarted = true;
      }
      continue;
    }

    int binaryPayloadLen = 0;
    if (imageStreamingStarted && parseImageBinaryHeader(line, binaryPayloadLen))
    {
      std::vector<uint8_t> chunk;
      if (!readSerialBytesExact(arduino, chunk, static_cast<size_t>(binaryPayloadLen), IMAGE_CAPTURE_LINE_TIMEOUT_MS))
      {
        std::cerr << "Timeout waiting for binary image chunk payload." << std::endl;
        return false;
      }
      if (!consumeImageFrameDelimiter(arduino, 2000))
      {
        ++delimiterErrorCount;
        if (delimiterErrorCount > 3)
        {
          std::cerr << "Invalid binary image chunk delimiter." << std::endl;
          return false;
        }
        drainSerialInput(arduino, 150);
        continue;
      }

      delimiterErrorCount = 0;

      imageBytes.insert(imageBytes.end(), chunk.begin(), chunk.end());
      ++packetCount;
      printTransferProgress("Downloading image:", imageBytes.size(), targetImageBytes, packetCount, transferStart);
      continue;
    }

    std::string hexPayload;
    if (imageStreamingStarted && parseImageLine(line, hexPayload))
    {
      const std::vector<uint8_t> bytes = parseHexBytes(hexPayload);
      imageBytes.insert(imageBytes.end(), bytes.begin(), bytes.end());
      ++packetCount;
      printTransferProgress("Downloading image:", imageBytes.size(), targetImageBytes, packetCount, transferStart);
      continue;
    }

    int cmd = 0;
    int status = STS_ERR;
    std::string payload;
    if (!parseResponseLine(line, cmd, status, payload))
    {
      continue;
    }

    if (cmd != CMD_CAPTURE_IMAGE)
    {
      continue;
    }

    if (packetCount > 0)
    {
      std::cout << std::endl;
    }
    std::cout << "Command " << CMD_CAPTURE_IMAGE << " status=" << status << std::endl;
    std::cout << "Data: " << payload << std::endl;
    targetImageBytes = parseTargetBytesFromPayload(payload, IMAGE_TARGET_PACKED_BYTES, true);
    printTransferSummary("Image transfer", packetCount, imageBytes.size(), transferStart);

    if (status != STS_OK)
    {
      return false;
    }

    // Parse optional payload attributes: width, height.
    const std::vector<std::string> attrs = split(payload, ';');
    for (size_t i = 0; i < attrs.size(); ++i)
    {
      const std::vector<std::string> kv = split(attrs[i], '=');
      if (kv.size() != 2)
      {
        continue;
      }
      if (kv[0] == "width")
      {
        imageWidth = std::atoi(kv[1].c_str());
      }
      else if (kv[0] == "height")
      {
        imageHeight = std::atoi(kv[1].c_str());
      }
      else if (kv[0] == "format")
      {
        imageFormat = kv[1];
      }
      else if (kv[0] == "full_height")
      {
        fullImageHeight = std::atoi(kv[1].c_str());
      }
    }

    const size_t expectedPixels = static_cast<size_t>(imageWidth) * static_cast<size_t>(imageHeight);
    std::vector<uint8_t> gray8Bytes;

    if (imageFormat == "gray4_packed")
    {
      const size_t expectedPacked = expectedPixels / 2U;
      if (imageBytes.size() < expectedPacked)
      {
        std::cerr << "Error: insufficient packed image payload. Got " << imageBytes.size()
                  << " bytes, expected at least " << expectedPacked << "." << std::endl;
        return false;
      }
      gray8Bytes = expandGray4PackedToGray8(imageBytes, imageWidth, imageHeight);

      if (fullImageHeight > imageHeight)
      {
        const size_t paddedPixels = static_cast<size_t>(imageWidth) * static_cast<size_t>(fullImageHeight);
        std::vector<uint8_t> padded(paddedPixels, 0xFFU);
        for (int y = 0; y < imageHeight; ++y)
        {
          const size_t srcOffset = static_cast<size_t>(y) * static_cast<size_t>(imageWidth);
          const size_t dstOffset = static_cast<size_t>(y) * static_cast<size_t>(imageWidth);
          for (int x = 0; x < imageWidth; ++x)
          {
            padded[dstOffset + static_cast<size_t>(x)] = gray8Bytes[srcOffset + static_cast<size_t>(x)];
          }
        }
        gray8Bytes.swap(padded);
        imageHeight = fullImageHeight;
      }
    }
    else
    {
      gray8Bytes = imageBytes;
    }

    if (gray8Bytes.size() < expectedPixels)
    {
      std::cerr << "Error: insufficient image payload for BMP export. Got " << gray8Bytes.size()
                << " bytes, expected at least " << expectedPixels << "." << std::endl;
      return false;
    }

    persistFingerprintImageBmp(gray8Bytes, imageWidth, imageHeight);
    return true;
  }
}

/**
 * @brief Print interactive service menu.
 */
static void printServiceMenu(const FlashConfig &config)
{
  std::cout << "\n========== Fingerprint Service Menu ==========\n";
  std::cout << "1) Read board status\n";
  std::cout << "2) Check fingerprint module status\n";
  std::cout << "3) Read/store fingerprint and export template\n";
  std::cout << "4) Capture fingerprint image\n";
  std::cout << "5) Quit\n";
  std::cout << "----------------------------------------------\n";
  std::cout << "Connected port: " << config.port << "\n";
  std::cout << "Select option [1-5]: ";
}

/**
 * @brief Main interactive service loop.
 */
static void runServiceMenu(SerialPort &arduino, const FlashConfig &config)
{
  while (true)
  {
    printServiceMenu(config);
    std::string choice;
    if (!std::getline(std::cin, choice))
    {
      break;
    }

    if (choice == "1")
    {
      (void)requestSimpleStatus(arduino, CMD_BOARD_STATUS);
    }
    else if (choice == "2")
    {
      (void)requestSimpleStatus(arduino, CMD_FP_STATUS);
    }
    else if (choice == "3")
    {
      (void)requestFingerprintReadStore(arduino);
    }
    else if (choice == "4")
    {
      (void)requestFingerprintImageCapture(arduino);
    }
    else if (choice == "5")
    {
      break;
    }
    else
    {
      std::cout << "Invalid option. Please choose 1, 2, 3, 4, or 5." << std::endl;
    }
  }
}

/**
 * @brief Entry point.
 */
int main(int argc, char **argv)
{
  FlashConfig flashConfig;
  const ParseStatus status = parseArguments(argc, argv, flashConfig);

  if (status == ParseStatus::Help)
  {
    printUsage(argv[0]);
    return EXIT_SUCCESS;
  }
  if (status == ParseStatus::Error)
  {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (flashConfig.flashMode)
  {
    return flashArduino(flashConfig);
  }

  SerialPort arduino;
  arduino.setup();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << currentDateTime() << "-->[Connecting to arduino]: "
            << (arduino.isConnected() ? "Success" : "Fail") << std::endl;

  if (!arduino.isConnected())
  {
    return EXIT_FAILURE;
  }

  waitForStartupEvents(arduino);

  runServiceMenu(arduino, flashConfig);
  return EXIT_SUCCESS;
}
