#ifndef LINKY_PARSER_H
#define LINKY_PARSER_H

#include <string>

// ============================================================
// MODES DE COMMUNICATION TIC LINKY
// ============================================================

enum class LinkyParserMode {
    Historique,
    Standard
};

// ============================================================
// DONNEES DECODEES DU COMPTEUR LINKY
// ============================================================

struct LinkyParsedData {

    // --------------------------------------------------------
    // IDENTIFICATION
    // --------------------------------------------------------

    std::string meterId;
    std::string tariffOption;
    std::string timestamp;

    // --------------------------------------------------------
    // ENERGIE (kWh)
    // --------------------------------------------------------

    // Consommation totale ou index BASE
    float consumptionKWh = 0.0f;

    // Index tarif Base
    float consumptionBaseKWh = 0.0f;

    // Heures creuses
    float consumptionHCKWh = 0.0f;

    // Heures pleines
    float consumptionHPKWh = 0.0f;

    // Energie injectee
    float productionKWh = 0.0f;

    // Index tarifaires du mode standard
    // Leur signification depend du contrat.
    float tariffIndex1KWh = 0.0f;
    float tariffIndex2KWh = 0.0f;

    // --------------------------------------------------------
    // PUISSANCE ACTIVE (kW)
    // --------------------------------------------------------

    // Ces champs sont conserves pour compatibilite.
    // Ils ne doivent pas recevoir directement PAPP,
    // SINSTS ou SINSTI (puissances apparentes).

    float powerConsumedKW = 0.0f;
    float powerProducedKW = 0.0f;

    // --------------------------------------------------------
    // PUISSANCE APPARENTE (kVA)
    // --------------------------------------------------------

    // PAPP ou SINSTS
    float apparentPowerConsumedKVA = 0.0f;

    // SINSTI
    float apparentPowerProducedKVA = 0.0f;

    // --------------------------------------------------------
    // MESURES ELECTRIQUES
    // --------------------------------------------------------

    float currentA = 0.0f;
    float voltageV = 0.0f;

    // --------------------------------------------------------
    // ABONNEMENT
    // --------------------------------------------------------

    float subscribedCurrentA = 0.0f;

    // --------------------------------------------------------
    // INDICATEURS DE PRESENCE
    // --------------------------------------------------------

    bool hasMeterId = false;
    bool hasTimestamp = false;

    // Energie
    bool hasConsumption = false;
    bool hasConsumptionBase = false;
    bool hasConsumptionHC = false;
    bool hasConsumptionHP = false;
    bool hasProduction = false;

    // Index tarifaires standard
    bool hasTariffIndex1 = false;
    bool hasTariffIndex2 = false;

    // Puissance active
    bool hasPowerConsumed = false;
    bool hasPowerProduced = false;

    // Puissance apparente
    bool hasApparentPowerConsumed = false;
    bool hasApparentPowerProduced = false;

    // Mesures
    bool hasCurrent = false;
    bool hasVoltage = false;
};

// ============================================================
// PARSER TIC LINKY
// ============================================================

class LinkyParser {

public:

    // Analyse un groupe TIC et met a jour les donnees.
    static bool parseLine(
        const std::string& line,
        LinkyParserMode mode,
        LinkyParsedData& data
    );

    // Verifie le checksum TIC.
    static bool validateChecksum(
        const std::string& line
    );

    // Calcule le checksum d'un groupe TIC.
    static char calculateChecksum(
        const std::string& content
    );

private:

    // Mode historique :
    // ETIQUETTE ESPACE VALEUR ESPACE CHECKSUM
    static bool extractHistoricalGroup(
        const std::string& line,
        std::string& label,
        std::string& value
    );

    // Mode standard :
    // ETIQUETTE TAB VALEUR TAB CHECKSUM
    //
    // Ou :
    // ETIQUETTE TAB HORODATE TAB VALEUR TAB CHECKSUM
    static bool extractStandardGroup(
        const std::string& line,
        std::string& label,
        std::string& timestamp,
        std::string& value
    );

    // Conversion texte vers nombre
    static float toFloat(
        const std::string& value
    );
};

#endif // LINKY_PARSER_H