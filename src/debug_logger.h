#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <Arduino.h>

// Log levels (as constants)
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

// Backward compatibility - keep old names but they may conflict with macros
#ifndef TEST_BUILD
  #define LOG_ERROR LOG_LEVEL_ERROR
  #define LOG_WARN  LOG_LEVEL_WARN
  #define LOG_INFO  LOG_LEVEL_INFO
  #define LOG_DEBUG LOG_LEVEL_DEBUG
#endif

// Module identifiers
#define MODULE_MAIN    "MAIN"
#define MODULE_P1      "P1"
#define MODULE_MQTT    "MQTT"
#define MODULE_WIFI    "WIFI"
#define MODULE_WEB     "WEB"
#define MODULE_OTA     "OTA"
#define MODULE_CONFIG  "CFG"

// Global log level (can be changed at runtime or compile time)
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO  // Default to INFO level
#endif

// Module filter - set to empty string to show all modules, or specific module name
#ifndef LOG_MODULE_FILTER
#define LOG_MODULE_FILTER ""  // Empty = show all modules
#endif

class DebugLogger {
public:
  static void log(uint8_t level, const char* module, const char* format, ...);
  
  // Convenience macros
  static void error(const char* module, const char* format, ...);
  static void warn(const char* module, const char* format, ...);
  static void info(const char* module, const char* format, ...);
  static void debug(const char* module, const char* format, ...);
  
  // Set log level at runtime
  static void setLevel(uint8_t level);
  
  // Set module filter at runtime
  static void setModuleFilter(const char* module);
  
private:
  static uint8_t currentLevel;
  static String moduleFilter;
  static const char* levelNames[];
  static void printTimestamp();
  static bool shouldLog(uint8_t level, const char* module);
};

// Convenience macros for easier usage
// In test builds, these replace the constants; in normal builds they're function macros
#ifdef TEST_BUILD
  // In test builds, define as function macros (constants already undefined above)
  #define LOG_ERROR(module, ...) DebugLogger::error(module, __VA_ARGS__)
  #define LOG_WARN(module, ...)  DebugLogger::warn(module, __VA_ARGS__)
  #define LOG_INFO(module, ...)  DebugLogger::info(module, __VA_ARGS__)
  #define LOG_DEBUG(module, ...) DebugLogger::debug(module, __VA_ARGS__)
#else
  // In normal builds, provide function-like macros (but constants still exist)
  #define LOG_ERROR_FUNC(module, ...) DebugLogger::error(module, __VA_ARGS__)
  #define LOG_WARN_FUNC(module, ...)  DebugLogger::warn(module, __VA_ARGS__)
  #define LOG_INFO_FUNC(module, ...)  DebugLogger::info(module, __VA_ARGS__)
  #define LOG_DEBUG_FUNC(module, ...) DebugLogger::debug(module, __VA_ARGS__)
  // For convenience, also define LOG_* as function macros (will shadow constants)
  #define LOG_ERROR(module, ...) DebugLogger::error(module, __VA_ARGS__)
  #define LOG_WARN(module, ...)  DebugLogger::warn(module, __VA_ARGS__)
  #define LOG_INFO(module, ...)  DebugLogger::info(module, __VA_ARGS__)
  #define LOG_DEBUG(module, ...) DebugLogger::debug(module, __VA_ARGS__)
#endif

#endif
