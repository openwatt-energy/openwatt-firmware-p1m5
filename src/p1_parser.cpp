#include "p1_parser.h"

#include <cstdlib>
#include <sstream>

// ============================================================
// CALCUL CRC16
// ============================================================

uint16_t P1Parser::calculateCRC16(const char* data, size_t len) {

    uint16_t crc = 0;

    for (size_t i = 0; i < len; i++) {

        crc ^= static_cast<uint8_t>(data[i]);

        for (int j = 0; j < 8; j++) {

            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}


// ============================================================
// VALIDATION CRC
// ============================================================

bool P1Parser::validateCRC(const std::string& telegram) {

    size_t crcIdx = telegram.find('!');

    // Vérifier la présence du caractère de fin
    if (crcIdx == std::string::npos) {
        return false;
    }

    // Vérifier la présence des 4 caractères CRC
    if (telegram.length() < crcIdx + 5) {
        return false;
    }

    std::string crcString = telegram.substr(crcIdx + 1, 4);

    uint16_t receivedCRC = 0;

    // Convertir le CRC hexadécimal
    for (char c : crcString) {

        uint8_t nibble;

        if (c >= '0' && c <= '9') {

            nibble = static_cast<uint8_t>(c - '0');

        } else if (c >= 'A' && c <= 'F') {

            nibble = static_cast<uint8_t>(c - 'A' + 10);

        } else if (c >= 'a' && c <= 'f') {

            nibble = static_cast<uint8_t>(c - 'a' + 10);

        } else {

            return false;
        }

        receivedCRC = static_cast<uint16_t>(
            (receivedCRC << 4) | nibble
        );
    }

    // Vérifier qu'il n'y a pas de données supplémentaires
    // après les 4 caractères CRC.
    // Seuls les caractères CR et LF sont autorisés.
    for (size_t i = crcIdx + 5; i < telegram.length(); i++) {

        if (telegram[i] != '\r' && telegram[i] != '\n') {
            return false;
        }
    }

    // Calculer le CRC jusqu'au caractère '!' inclus
    std::string telegramWithoutCRC =
        telegram.substr(0, crcIdx + 1);

    uint16_t calculatedCRC = calculateCRC16(
        telegramWithoutCRC.c_str(),
        telegramWithoutCRC.length()
    );

    // Comparer le CRC reçu et le CRC calculé
    return calculatedCRC == receivedCRC;
}


// ============================================================
// EXTRACTION D'UNE VALEUR TEXTE
// ============================================================

std::string P1Parser::extractStringValue(const std::string& line) {

    size_t start = line.find('(');
    size_t end = line.find(')', start);

    if (start == std::string::npos ||
        end == std::string::npos ||
        end <= start) {

        return "";
    }

    return line.substr(
        start + 1,
        end - start - 1
    );
}


// ============================================================
// EXTRACTION D'UNE VALEUR FLOAT
// ============================================================

float P1Parser::extractFloatValue(const std::string& line) {

    size_t start = line.find('(');

    if (start == std::string::npos) {
        return 0.0f;
    }

    size_t end = line.find('*', start);

    if (end == std::string::npos) {
        end = line.find(')', start);
    }

    if (end == std::string::npos || end <= start) {
        return 0.0f;
    }

    std::string value =
        line.substr(
            start + 1,
            end - start - 1
        );

    return std::strtof(
        value.c_str(),
        nullptr
    );
}


// ============================================================
// EXTRACTION D'UNE VALEUR ENTIÈRE
// ============================================================

int P1Parser::extractIntValue(const std::string& line) {

    std::string value = extractStringValue(line);

    if (value.empty()) {
        return 0;
    }

    return std::atoi(value.c_str());
}


// ============================================================
// PARSEUR PRINCIPAL P1
// ============================================================

P1ParsedData P1Parser::parse(const std::string& telegram) {

    P1ParsedData data;

    // --------------------------------------------------------
    // 1. Vérification de la taille
    // --------------------------------------------------------

    const size_t MAX_TELEGRAM_SIZE = 2048;

    if (telegram.size() > MAX_TELEGRAM_SIZE) {
        return data;
    }

    // --------------------------------------------------------
    // 2. Vérification du début du télégramme
    // --------------------------------------------------------

    if (telegram.empty() || telegram[0] != '/') {
        return data;
    }

    // --------------------------------------------------------
    // 3. Vérification CRC
    // --------------------------------------------------------

    if (!validateCRC(telegram)) {
        return data;
    }

    // --------------------------------------------------------
    // 4. Normalisation CR/LF
    // --------------------------------------------------------

    std::string normalized;

    for (size_t i = 0; i < telegram.size(); i++) {

        if (telegram[i] == '\r') {

            if (i + 1 < telegram.size() &&
                telegram[i + 1] == '\n') {

                continue;
            }

            normalized += '\n';

        } else {

            normalized += telegram[i];
        }
    }

    // --------------------------------------------------------
    // 5. Lecture des lignes
    // --------------------------------------------------------

    std::istringstream stream(normalized);

    std::string line;

    while (std::getline(stream, line)) {

        // Ignorer les lignes vides et la ligne CRC
        if (line.empty() || line[0] == '!') {
            continue;
        }

        // ----------------------------------------------------
        // Identifiant du compteur
        // ----------------------------------------------------

        if (line.find("0-0:96.1.1(") != std::string::npos) {

            data.equipmentId = extractStringValue(line);
        }

        // ----------------------------------------------------
        // Date et heure
        // ----------------------------------------------------

        else if (line.find("0-0:1.0.0(") != std::string::npos) {

            data.timestamp = extractStringValue(line);
        }

        // ----------------------------------------------------
        // Tarif actif
        // ----------------------------------------------------

        else if (line.find("0-0:96.14.0(") != std::string::npos) {

            data.tariffIndicator =
                static_cast<uint8_t>(
                    extractIntValue(line)
                );
        }

        // ----------------------------------------------------
        // Consommation tarif 1
        // ----------------------------------------------------

        else if (
            line.find("1-0:1.8.1(") != std::string::npos ||
            line.find("1-0:1.8.0(") != std::string::npos
        ) {

            data.consumptionT1 = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Consommation tarif 2
        // ----------------------------------------------------

        else if (line.find("1-0:1.8.2(") != std::string::npos) {

            data.consumptionT2 = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Production tarif 1
        // ----------------------------------------------------

        else if (
            line.find("1-0:2.8.1(") != std::string::npos ||
            line.find("1-0:2.8.0(") != std::string::npos
        ) {

            data.productionT1 = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Production tarif 2
        // ----------------------------------------------------

        else if (line.find("1-0:2.8.2(") != std::string::npos) {

            data.productionT2 = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Puissance consommée
        // ----------------------------------------------------

        else if (line.find("1-0:1.7.0(") != std::string::npos) {

            data.powerConsumed = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Puissance produite
        // ----------------------------------------------------

        else if (line.find("1-0:2.7.0(") != std::string::npos) {

            data.powerProduced = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Demande moyenne
        // ----------------------------------------------------

        else if (line.find("1-0:1.4.0(") != std::string::npos) {

            data.avgDemand = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Demande maximale sur 13 mois
        // ----------------------------------------------------

        else if (line.find("0-0:98.1.0(") != std::string::npos) {

            data.maxDemand13M = extractFloatValue(line);
        }

        // ----------------------------------------------------
        // Demande maximale du mois
        //
        // Format :
        // 1-0:1.6.0(timestamp)(value*kW)
        // ----------------------------------------------------

        else if (line.find("1-0:1.6.0(") != std::string::npos) {

            size_t firstClose = line.find(')');

            if (firstClose != std::string::npos) {

                size_t secondOpen =
                    line.find('(', firstClose + 1);

                if (secondOpen != std::string::npos) {

                    size_t star =
                        line.find('*', secondOpen);

                    if (star != std::string::npos &&
                        star > secondOpen) {

                        std::string value =
                            line.substr(
                                secondOpen + 1,
                                star - secondOpen - 1
                            );

                        data.maxDemandMonth =
                            std::strtof(
                                value.c_str(),
                                nullptr
                            );
                    }
                }
            }
        }
    }

    // --------------------------------------------------------
    // 6. Vérifier la présence de données utiles
    // --------------------------------------------------------

    bool hasAnyData =
        !data.equipmentId.empty() ||
        !data.timestamp.empty() ||
        data.consumptionT1 > 0.0f ||
        data.consumptionT2 > 0.0f ||
        data.productionT1 > 0.0f ||
        data.productionT2 > 0.0f;

    // --------------------------------------------------------
    // 7. Résultat final
    // --------------------------------------------------------

    data.valid = hasAnyData;

    return data;
}