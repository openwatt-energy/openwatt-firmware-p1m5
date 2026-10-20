#include "linky_parser.h"

#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <cctype>

// ============================================================
// CONVERSION NUMERIQUE
// ============================================================

float LinkyParser::toFloat(const std::string& value) {
    if (value.empty()) {
        return NAN;
    }

    errno = 0;
    char* end = nullptr;

    float result = std::strtof(value.c_str(), &end);

    if (end == value.c_str() || errno == ERANGE) {
        return NAN;
    }

    // Autoriser uniquement des espaces en fin de valeur.
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return NAN;
        }
        ++end;
    }

    if (!std::isfinite(result)) {
        return NAN;
    }

    return result;
}

// ============================================================
// CHECKSUM TIC
// ============================================================

char LinkyParser::calculateChecksum(
    const std::string& content
) {
    unsigned int sum = 0;

    for (unsigned char c : content) {
        sum += c;
    }

    return static_cast<char>((sum & 0x3F) + 0x20);
}

bool LinkyParser::validateChecksum(
    const std::string& line
) {
    if (line.size() < 3) {
        return false;
    }

    const char receivedChecksum = line.back();

    const std::string content =
        line.substr(0, line.size() - 1);

    if (content.empty()) {
        return false;
    }

    const char separator = content.back();

    if (separator != ' ' && separator != '\t') {
        return false;
    }

    return calculateChecksum(content) == receivedChecksum;
}

// ============================================================
// EXTRACTION MODE HISTORIQUE
// ============================================================

bool LinkyParser::extractHistoricalGroup(
    const std::string& line,
    std::string& label,
    std::string& value
) {
    if (line.size() < 3) {
        return false;
    }

    const std::string content =
        line.substr(0, line.size() - 2);

    const size_t separator = content.find(' ');

    if (separator == std::string::npos) {
        return false;
    }

    label = content.substr(0, separator);

    const size_t valueStart =
        content.find_first_not_of(' ', separator);

    if (valueStart == std::string::npos) {
        return false;
    }

    value = content.substr(valueStart);

    return !label.empty() && !value.empty();
}

// ============================================================
// EXTRACTION MODE STANDARD
// ============================================================

bool LinkyParser::extractStandardGroup(
    const std::string& line,
    std::string& label,
    std::string& timestamp,
    std::string& value
) {
    label.clear();
    timestamp.clear();
    value.clear();

    if (line.size() < 3) {
        return false;
    }

    const std::string content =
        line.substr(0, line.size() - 2);

    const size_t firstTab = content.find('\t');

    if (firstTab == std::string::npos) {
        return false;
    }

    label = content.substr(0, firstTab);

    if (label.empty()) {
        return false;
    }

    const size_t secondTab =
        content.find('\t', firstTab + 1);

    if (secondTab == std::string::npos) {
        value = content.substr(firstTab + 1);
        return !value.empty();
    }

    timestamp = content.substr(
        firstTab + 1,
        secondTab - firstTab - 1
    );

    value = content.substr(secondTab + 1);

    return !timestamp.empty() && !value.empty();
}

// ============================================================
// PARSER PRINCIPAL
// ============================================================

bool LinkyParser::parseLine(
    const std::string& line,
    LinkyParserMode mode,
    LinkyParsedData& data
) {
    if (!validateChecksum(line)) {
        return false;
    }

    std::string label;
    std::string value;
    std::string timestamp;

    // --------------------------------------------------------
    // EXTRACTION SELON LE MODE
    // --------------------------------------------------------

    if (mode == LinkyParserMode::Historique) {
        if (!extractHistoricalGroup(line, label, value)) {
            return false;
        }
    }
    else if (mode == LinkyParserMode::Standard) {
        if (!extractStandardGroup(
                line, label, timestamp, value)) {
            return false;
        }
    }
    else {
        return false;
    }

    // --------------------------------------------------------
    // VALIDATION ET CONVERSION NUMERIQUE
    // --------------------------------------------------------

    auto readNumber = [&](float& number) -> bool {
        number = toFloat(value);
        return std::isfinite(number) && number >= 0.0f;
    };

    auto saveTimestamp = [&]() {
        if (!timestamp.empty()) {
            data.timestamp = timestamp;
            data.hasTimestamp = true;
        }
    };

    float number = 0.0f;

    // ========================================================
    // MODE HISTORIQUE
    // ========================================================

    if (mode == LinkyParserMode::Historique) {

        // Identifiant compteur
        if (label == "ADCO") {
            data.meterId = value;
            data.hasMeterId = true;
            return true;
        }

        // Option tarifaire
        if (label == "OPTARIF") {
            data.tariffOption = value;
            return true;
        }

        // Tarif Base : Wh -> kWh
        if (label == "BASE") {
            if (!readNumber(number)) return false;

            data.consumptionBaseKWh = number / 1000.0f;
            data.hasConsumptionBase = true;

            data.consumptionKWh = data.consumptionBaseKWh;
            data.hasConsumption = true;

            return true;
        }

        // Heures creuses : Wh -> kWh
        if (label == "HCHC") {
            if (!readNumber(number)) return false;

            data.consumptionHCKWh = number / 1000.0f;
            data.hasConsumptionHC = true;

            if (data.hasConsumptionHP) {
                data.consumptionKWh =
                    data.consumptionHCKWh +
                    data.consumptionHPKWh;

                data.hasConsumption = true;
            }

            return true;
        }

        // Heures pleines : Wh -> kWh
        if (label == "HCHP") {
            if (!readNumber(number)) return false;

            data.consumptionHPKWh = number / 1000.0f;
            data.hasConsumptionHP = true;

            if (data.hasConsumptionHC) {
                data.consumptionKWh =
                    data.consumptionHCKWh +
                    data.consumptionHPKWh;

                data.hasConsumption = true;
            }

            return true;
        }

        // Puissance apparente soutiree : VA -> kVA
        if (label == "PAPP") {
            if (!readNumber(number)) return false;

            data.apparentPowerConsumedKVA =
                number / 1000.0f;

            data.hasApparentPowerConsumed = true;
            return true;
        }

        // Courant instantane : A
        if (label == "IINST") {
            if (!readNumber(number)) return false;

            data.currentA = number;
            data.hasCurrent = true;
            return true;
        }

        // Intensite souscrite : A
        if (label == "ISOUSC") {
            if (!readNumber(number)) return false;

            data.subscribedCurrentA = number;
            return true;
        }

        return false;
    }

    // ========================================================
    // MODE STANDARD
    // ========================================================

    if (mode == LinkyParserMode::Standard) {

        // Identifiant compteur
        if (label == "ADSC") {
            data.meterId = value;
            data.hasMeterId = true;
            saveTimestamp();
            return true;
        }

        // Energie active soutiree totale : Wh -> kWh
        if (label == "EAST") {
            if (!readNumber(number)) return false;

            data.consumptionKWh = number / 1000.0f;
            data.hasConsumption = true;

            saveTimestamp();
            return true;
        }

        // Energie active injectee : Wh -> kWh
        if (label == "EAIT") {
            if (!readNumber(number)) return false;

            data.productionKWh = number / 1000.0f;
            data.hasProduction = true;

            saveTimestamp();
            return true;
        }

        // Index tarifaire 1 : Wh -> kWh
        if (label == "EASF01") {
            if (!readNumber(number)) return false;

            data.tariffIndex1KWh = number / 1000.0f;
            data.hasTariffIndex1 = true;

            saveTimestamp();
            return true;
        }

        // Index tarifaire 2 : Wh -> kWh
        if (label == "EASF02") {
            if (!readNumber(number)) return false;

            data.tariffIndex2KWh = number / 1000.0f;
            data.hasTariffIndex2 = true;

            saveTimestamp();
            return true;
        }

        // Puissance apparente soutiree : VA -> kVA
        if (label == "SINSTS") {
            if (!readNumber(number)) return false;

            data.apparentPowerConsumedKVA =
                number / 1000.0f;

            data.hasApparentPowerConsumed = true;

            saveTimestamp();
            return true;
        }

        // Puissance apparente injectee : VA -> kVA
        if (label == "SINSTI") {
            if (!readNumber(number)) return false;

            data.apparentPowerProducedKVA =
                number / 1000.0f;

            data.hasApparentPowerProduced = true;

            saveTimestamp();
            return true;
        }

        // Tension phase 1 : V
        if (label == "URMS1") {
            if (!readNumber(number)) return false;

            data.voltageV = number;
            data.hasVoltage = true;

            saveTimestamp();
            return true;
        }

        // Courant phase 1 : A
        if (label == "IRMS1") {
            if (!readNumber(number)) return false;

            data.currentA = number;
            data.hasCurrent = true;

            saveTimestamp();
            return true;
        }

        return false;
    }

    return false;
}