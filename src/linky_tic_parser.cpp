#include "linky_tic_parser.h"

// ============================================================================
// CONSTRUCTEUR
// ============================================================================

LinkyTicParser::LinkyTicParser()
    : _serial(nullptr),
      _mode(LINKY_MODE_HISTORIQUE),
      _buffer(""),
      _lastValidDataMillis(0) {
}

// ============================================================================
// INITIALISATION
// ============================================================================

void LinkyTicParser::begin(
    HardwareSerial& serial,
    LinkyMode mode
) {
    _serial = &serial;
    _mode = mode;
    _buffer = "";
    _lastValidDataMillis = 0;

    // Réinitialiser les mesures lors d'une nouvelle initialisation.
    _parsedData = LinkyParsedData{};

    // Historique : 1200 bauds, 7 bits, parité paire, 1 stop bit.
    if (_mode == LINKY_MODE_HISTORIQUE) {
        _serial->begin(1200, SERIAL_7E1);
    }
    // Standard : 9600 bauds, 7 bits, parité paire, 1 stop bit.
    else {
        _serial->begin(9600, SERIAL_7E1);
    }
}

// ============================================================================
// LECTURE UART
// ============================================================================

bool LinkyTicParser::update(
    JsonDocument& liveDataDoc
) {
    if (_serial == nullptr) {
        return false;
    }

    bool newData = false;

    // Évite de conserver un groupe TIC anormalement long.
    constexpr size_t MAX_TIC_LINE_LENGTH = 512;

    while (_serial->available()) {

        const char c =
            static_cast<char>(_serial->read());

        // --------------------------------------------------------------------
        // STX : début de trame TIC
        // --------------------------------------------------------------------

        if (c == 0x02) {
            _buffer = "";
            continue;
        }

        // --------------------------------------------------------------------
        // CR : ignoré
        // --------------------------------------------------------------------

        if (c == '\r') {
            continue;
        }

        // --------------------------------------------------------------------
        // LF : fin d'un groupe TIC
        // --------------------------------------------------------------------

        if (c == '\n') {

            if (_buffer.length() > 0 &&
                _buffer.length() <= MAX_TIC_LINE_LENGTH) {

                if (processLine(_buffer, liveDataDoc)) {
                    newData = true;
                }
            }

            _buffer = "";
            continue;
        }

        // --------------------------------------------------------------------
        // ETX : fin de trame TIC
        // --------------------------------------------------------------------

        if (c == 0x03) {

            if (_buffer.length() > 0 &&
                _buffer.length() <= MAX_TIC_LINE_LENGTH) {

                if (processLine(_buffer, liveDataDoc)) {
                    newData = true;
                }
            }

            _buffer = "";
            continue;
        }

        // --------------------------------------------------------------------
        // Caractères normaux
        // --------------------------------------------------------------------

        // Une ligne trop longue est ignorée jusqu'au prochain
        // séparateur LF, ETX ou STX.
        //
        // Le caractère '\x01' sert ici de marqueur d'erreur
        // pour éviter d'ajouter la suite d'une ligne invalide.

        if (_buffer.length() == 1 && _buffer[0] == '\x01') {
            continue;
        }

        if (_buffer.length() >= MAX_TIC_LINE_LENGTH) {
            _buffer = "\x01";
            continue;
        }

        _buffer += c;
    }

    return newData;
}

// ============================================================================
// TRAITEMENT D'UN GROUPE TIC
// ============================================================================

bool LinkyTicParser::processLine(
    const String& line,
    JsonDocument& doc
) {
    if (line.length() == 0) {
        return false;
    }

    // Conversion Arduino String -> std::string.
    const std::string ticLine(
        line.c_str(),
        line.length()
    );

    // Conversion du mode TIC.
    const LinkyParserMode parserMode =
        (_mode == LINKY_MODE_HISTORIQUE)
            ? LinkyParserMode::Historique
            : LinkyParserMode::Standard;

    // Vérification du checksum et décodage.
    const bool parsed = LinkyParser::parseLine(
        ticLine,
        parserMode,
        _parsedData
    );

    if (!parsed) {
        return false;
    }

    // Mise à jour de la dernière réception valide.
    _lastValidDataMillis = millis();

    // Conversion vers le format utilisé par le firmware.
    updateJsonDocument(doc);

    return true;
}

// ============================================================================
// ÉTAT DE CONNEXION LINKY
// ============================================================================

bool LinkyTicParser::isConnected() const {

    if (_lastValidDataMillis == 0) {
        return false;
    }

    return (millis() - _lastValidDataMillis)
        < CONNECTION_TIMEOUT_MS;
}

// ============================================================================
// CONVERSION LINKY -> FORMAT INTERNE
// ============================================================================

void LinkyTicParser::updateJsonDocument(
    JsonDocument& doc
) {

    // ------------------------------------------------------------------------
    // HORODATAGE
    // ------------------------------------------------------------------------

    if (_parsedData.hasTimestamp) {
        doc["0-0:1.0.0"] =
            _parsedData.timestamp.c_str();
    }

    // ------------------------------------------------------------------------
    // IDENTIFIANT COMPTEUR
    // ------------------------------------------------------------------------

    if (_parsedData.hasMeterId) {
        doc["0-0:96.1.1"] =
            _parsedData.meterId.c_str();
    }

    // ------------------------------------------------------------------------
    // OPTION TARIFAIRE
    // ------------------------------------------------------------------------

    if (!_parsedData.tariffOption.empty()) {
        doc["0-0:96.14.0"] =
            _parsedData.tariffOption.c_str();
    }

    // ------------------------------------------------------------------------
    // ÉNERGIE TOTALE SOUTIRÉE (kWh)
    // ------------------------------------------------------------------------

    if (_parsedData.hasConsumption) {
        doc["1-0:1.8.0"] =
            _parsedData.consumptionKWh;
    }

    // ------------------------------------------------------------------------
    // INDEX TARIF BASE (kWh)
    // ------------------------------------------------------------------------

    // Conservation dans un champ explicite.
    // Ne pas confondre avec un index HP ou HC.

    if (_parsedData.hasConsumptionBase) {
        doc["linky_base_kwh"] =
            _parsedData.consumptionBaseKWh;
    }

    // ------------------------------------------------------------------------
    // HEURES CREUSES (kWh)
    // ------------------------------------------------------------------------

    if (_parsedData.hasConsumptionHC) {
        doc["linky_hc_kwh"] =
            _parsedData.consumptionHCKWh;
    }

    // ------------------------------------------------------------------------
    // HEURES PLEINES (kWh)
    // ------------------------------------------------------------------------

    if (_parsedData.hasConsumptionHP) {
        doc["linky_hp_kwh"] =
            _parsedData.consumptionHPKWh;
    }

    // ------------------------------------------------------------------------
    // INDEX TARIFAIRES STANDARD (kWh)
    // ------------------------------------------------------------------------

    // EASF01 et EASF02 ne correspondent pas systématiquement
    // aux heures creuses et aux heures pleines.

    if (_parsedData.hasTariffIndex1) {
        doc["linky_easf01_kwh"] =
            _parsedData.tariffIndex1KWh;
    }

    if (_parsedData.hasTariffIndex2) {
        doc["linky_easf02_kwh"] =
            _parsedData.tariffIndex2KWh;
    }

    // ------------------------------------------------------------------------
    // ÉNERGIE INJECTÉE (kWh)
    // ------------------------------------------------------------------------

    if (_parsedData.hasProduction) {
        doc["1-0:2.8.0"] =
            _parsedData.productionKWh;
    }

    // ------------------------------------------------------------------------
    // PUISSANCE ACTIVE SOUTIRÉE (kW)
    // ------------------------------------------------------------------------

    // Ne jamais remplir ce champ avec PAPP ou SINSTS :
    // ces étiquettes représentent des puissances apparentes.

    if (_parsedData.hasPowerConsumed) {
        doc["1-0:1.7.0"] =
            _parsedData.powerConsumedKW;
    }

    // ------------------------------------------------------------------------
    // PUISSANCE ACTIVE INJECTÉE (kW)
    // ------------------------------------------------------------------------

    if (_parsedData.hasPowerProduced) {
        doc["1-0:2.7.0"] =
            _parsedData.powerProducedKW;
    }

    // ------------------------------------------------------------------------
    // PUISSANCE APPARENTE SOUTIRÉE (kVA)
    // ------------------------------------------------------------------------

    if (_parsedData.hasApparentPowerConsumed) {
        doc["1-0:9.7.0"] =
            _parsedData.apparentPowerConsumedKVA;
    }

    // ------------------------------------------------------------------------
    // PUISSANCE APPARENTE INJECTÉE (kVA)
    // ------------------------------------------------------------------------

    // Champ explicite : ne pas assimiler la puissance
    // apparente injectée à la puissance active injectée.

    if (_parsedData.hasApparentPowerProduced) {
        doc["linky_apparent_power_injected_kva"] =
            _parsedData.apparentPowerProducedKVA;
    }

    // ------------------------------------------------------------------------
    // COURANT PHASE 1 (A)
    // ------------------------------------------------------------------------

    if (_parsedData.hasCurrent) {
        doc["1-0:31.7.0"] =
            _parsedData.currentA;
    }

    // ------------------------------------------------------------------------
    // TENSION PHASE 1 (V)
    // ------------------------------------------------------------------------

    if (_parsedData.hasVoltage) {
        doc["1-0:32.7.0"] =
            _parsedData.voltageV;
    }

    // ------------------------------------------------------------------------
    // COURANT SOUSCRIT (A)
    // ------------------------------------------------------------------------

    if (_parsedData.subscribedCurrentA > 0.0f) {
        doc["1-0:94.32.1"] =
            _parsedData.subscribedCurrentA;
    }
}