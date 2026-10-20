#ifndef LINKY_TIC_PARSER_H
#define LINKY_TIC_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "linky_parser.h"

// Modes TIC Linky
enum LinkyMode {
    LINKY_MODE_HISTORIQUE = 0,
    LINKY_MODE_STANDARD = 1
};

class LinkyTicParser {

public:

    LinkyTicParser();

    // Initialise le port série utilisé pour la TIC Linky
    void begin(
        HardwareSerial& serial,
        LinkyMode mode = LINKY_MODE_HISTORIQUE
    );

    // Lit les données disponibles sur le port série.
    // Retourne true si au moins un nouveau groupe TIC valide
    // a été reçu et parsé.
    bool update(
        JsonDocument& liveDataDoc
    );

    // Retourne true si une donnée TIC valide
    // a été reçue récemment.
    bool isConnected() const;

private:

    HardwareSerial* _serial;

    LinkyMode _mode;

    // Buffer contenant le groupe TIC en cours de réception
    String _buffer;

    // Dernières données Linky parsées
    LinkyParsedData _parsedData;

    // Heure de réception du dernier groupe TIC valide
    unsigned long _lastValidDataMillis;

    // Après ce délai sans donnée valide,
    // le compteur est considéré comme déconnecté.
    static const unsigned long CONNECTION_TIMEOUT_MS = 10000;

    bool processLine(
        const String& line,
        JsonDocument& doc
    );

    void updateJsonDocument(
        JsonDocument& doc
    );
};

#endif