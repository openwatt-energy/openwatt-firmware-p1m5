#ifndef P1_POWER_VALUE_H
#define P1_POWER_VALUE_H

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

// Totals are unsigned kW values. A missing/malformed field is not a zero.
inline bool parseP1UnsignedValue(const char* line, const char* prefix, const char* suffix, float& value) {
  const size_t prefixLength = std::strlen(prefix);
  if (std::strncmp(line, prefix, prefixLength) != 0) return false;

  const char* number = line + prefixLength;
  const char* cursor = number;
  if (*cursor < '0' || *cursor > '9') return false;
  while (*cursor >= '0' && *cursor <= '9') ++cursor;
  if (*cursor == '.') {
    ++cursor;
    if (*cursor < '0' || *cursor > '9') return false;
    while (*cursor >= '0' && *cursor <= '9') ++cursor;
  }
  if (std::strcmp(cursor, suffix) != 0) return false;

  errno = 0;
  char* end = nullptr;
  const float parsed = std::strtof(number, &end);
  if (end != cursor || errno == ERANGE || !std::isfinite(parsed)) return false;
  value = parsed;
  return true;
}

inline bool parseP1PowerValue(const char* line, const char* prefix, float& value) {
  return parseP1UnsignedValue(line, prefix, "*kW)", value);
}

#endif
