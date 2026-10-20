#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <Arduino.h>


// ============================================================================
// LOG LEVELS
// ============================================================================
//
// Valeurs numériques utilisées pour définir le niveau de log.
//
// Exemple :
//   LOG_LEVEL_ERROR = 0
//   LOG_LEVEL_WARN  = 1
//   LOG_LEVEL_INFO  = 2
//   LOG_LEVEL_DEBUG = 3
//

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3


// ============================================================================
// MODULE IDENTIFIERS
// ============================================================================

#define MODULE_MAIN    "MAIN"
#define MODULE_P1      "P1"
#define MODULE_MQTT    "MQTT"
#define MODULE_WIFI    "WIFI"
#define MODULE_WEB     "WEB"
#define MODULE_OTA     "OTA"
#define MODULE_CONFIG  "CFG"


// ============================================================================
// GLOBAL LOG LEVEL
// ============================================================================
//
// Peut être remplacé au moment de la compilation.
//
// Par défaut : INFO
//

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif


// ============================================================================
// MODULE FILTER
// ============================================================================
//
// Chaîne vide : afficher tous les modules.
//
// Exemple pour afficher uniquement MQTT :
//
//   #define LOG_MODULE_FILTER "MQTT"
//

#ifndef LOG_MODULE_FILTER
#define LOG_MODULE_FILTER ""
#endif


// ============================================================================
// DEBUG LOGGER
// ============================================================================

class DebugLogger {

public:

    static void log(
        uint8_t level,
        const char* module,
        const char* format,
        ...
    );


    // ------------------------------------------------------------------------
    // Convenience functions
    // ------------------------------------------------------------------------

    static void error(
        const char* module,
        const char* format,
        ...
    );

    static void warn(
        const char* module,
        const char* format,
        ...
    );

    static void info(
        const char* module,
        const char* format,
        ...
    );

    static void debug(
        const char* module,
        const char* format,
        ...
    );


    // ------------------------------------------------------------------------
    // Runtime configuration
    // ------------------------------------------------------------------------

    static void setLevel(uint8_t level);

    static void setModuleFilter(const char* module);


private:

    static uint8_t currentLevel;

    static String moduleFilter;

    static const char* levelNames[];

    static void printTimestamp();

    static bool shouldLog(
        uint8_t level,
        const char* module
    );
};


// ============================================================================
// CONVENIENCE MACROS
// ============================================================================
//
// IMPORTANT:
//
// LOG_LEVEL_* = constantes numériques
//
// LOG_*() = fonctions de journalisation
//
// Exemple :
//
//   LOG_INFO(MODULE_MAIN, "Firmware started");
//
//   LOG_ERROR(MODULE_WIFI, "Connection failed");
//
// Il n'y a donc plus de conflit entre les constantes et les macros.
//

#define LOG_ERROR(module, ...) \
    DebugLogger::error(module, __VA_ARGS__)

#define LOG_WARN(module, ...) \
    DebugLogger::warn(module, __VA_ARGS__)

#define LOG_INFO(module, ...) \
    DebugLogger::info(module, __VA_ARGS__)

#define LOG_DEBUG(module, ...) \
    DebugLogger::debug(module, __VA_ARGS__)


#endif