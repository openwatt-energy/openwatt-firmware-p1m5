#include "debug_logger.h"
#include <stdio.h>
#include <stdarg.h>

// Use the numeric constant directly to avoid macro expansion issues
uint8_t DebugLogger::currentLevel = LOG_LEVEL_INFO;
String DebugLogger::moduleFilter = String(LOG_MODULE_FILTER);

const char* DebugLogger::levelNames[] = {
  "ERROR",
  "WARN ",
  "INFO ",
  "DEBUG"
};

void DebugLogger::setLevel(uint8_t level) {
  currentLevel = level;
}

void DebugLogger::setModuleFilter(const char* module) {
  moduleFilter = String(module);
}

bool DebugLogger::shouldLog(uint8_t level, const char* module) {
  // Check log level
  if (level > currentLevel) {
    return false;
  }
  
  // Check module filter
  if (moduleFilter.length() > 0 && moduleFilter != String(module)) {
    return false;
  }
  
  return true;
}

void DebugLogger::printTimestamp() {
  unsigned long ms = millis();
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "[%02lu:%02lu:%02lu.%03lu]", 
           hours, minutes, seconds, ms % 1000);
  Serial.print(buffer);
}

void DebugLogger::log(uint8_t level, const char* module, const char* format, ...) {
  if (!shouldLog(level, module)) {
    return;
  }
  
  printTimestamp();
  Serial.print(" [");
  Serial.print(levelNames[level]);
  Serial.print("] [");
  Serial.print(module);
  Serial.print("] ");
  
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  Serial.println(buffer);
}

void DebugLogger::error(const char* module, const char* format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_ERROR, module, "%s", buffer);
}

void DebugLogger::warn(const char* module, const char* format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_WARN, module, "%s", buffer);
}

void DebugLogger::info(const char* module, const char* format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_INFO, module, "%s", buffer);
}

void DebugLogger::debug(const char* module, const char* format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  log(LOG_LEVEL_DEBUG, module, "%s", buffer);
}
