#include <unity.h>
#include <string>

#include "linky_parser.h"

// ============================================================================
// OUTIL DE TEST
// ============================================================================

// Construit automatiquement une ligne TIC avec un checksum valide.
//
// Historique : "ADCO 012345678901 "
// Standard   : "EAST\t001500000\t"
//
// Le contenu doit inclure le separateur avant le checksum.

static std::string makeValidTicLine(const std::string& content) {
    char checksum = LinkyParser::calculateChecksum(content);
    return content + checksum;
}

// ============================================================================
// TEST 1 - MODE HISTORIQUE
// ============================================================================

void test_linky_historique() {

    LinkyParsedData data;

    // ADCO - Identifiant du compteur
    std::string adco =
        makeValidTicLine("ADCO 012345678901 ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            adco,
            LinkyParserMode::Historique,
            data
        )
    );

    // OPTARIF - Option tarifaire
    std::string optarif =
        makeValidTicLine("OPTARIF BASE ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            optarif,
            LinkyParserMode::Historique,
            data
        )
    );

    // BASE - Energie consommee
    // 1 234 567 Wh = 1234.567 kWh
    std::string base =
        makeValidTicLine("BASE 001234567 ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            base,
            LinkyParserMode::Historique,
            data
        )
    );

    // PAPP - Puissance apparente
    // 450 VA = 0.450 kVA
    std::string papp =
        makeValidTicLine("PAPP 00450 ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            papp,
            LinkyParserMode::Historique,
            data
        )
    );

    // IINST - Courant instantane
    std::string iinst =
        makeValidTicLine("IINST 003 ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            iinst,
            LinkyParserMode::Historique,
            data
        )
    );

    // ISOUSC - Intensite souscrite
    std::string isousc =
        makeValidTicLine("ISOUSC 030 ");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            isousc,
            LinkyParserMode::Historique,
            data
        )
    );

    // Verification de l'identifiant
    TEST_ASSERT_TRUE(data.hasMeterId);

    TEST_ASSERT_EQUAL_STRING(
        "012345678901",
        data.meterId.c_str()
    );

    // Verification du tarif
    TEST_ASSERT_EQUAL_STRING(
        "BASE",
        data.tariffOption.c_str()
    );

    // Verification de l'energie
    TEST_ASSERT_TRUE(data.hasConsumption);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        1234.567f,
        data.consumptionKWh
    );

    // Verification de la puissance apparente
    TEST_ASSERT_TRUE(
        data.hasApparentPowerConsumed
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        0.450f,
        data.apparentPowerConsumedKVA
    );

    // PAPP ne fournit pas une puissance active en kW.
    TEST_ASSERT_FALSE(
        data.hasPowerConsumed
    );

    // Verification du courant
    TEST_ASSERT_TRUE(data.hasCurrent);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        3.0f,
        data.currentA
    );

    // Verification de l'intensite souscrite
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        30.0f,
        data.subscribedCurrentA
    );
}

// ============================================================================
// TEST 2 - MODE STANDARD
// ============================================================================

void test_linky_standard() {

    LinkyParsedData data;

    // EAST - Energie active soutiree
    // 1 500 000 Wh = 1500 kWh
    std::string east =
        makeValidTicLine("EAST\t001500000\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            east,
            LinkyParserMode::Standard,
            data
        )
    );

    // EAIT - Energie active injectee
    // 250 000 Wh = 250 kWh
    std::string eait =
        makeValidTicLine("EAIT\t000250000\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            eait,
            LinkyParserMode::Standard,
            data
        )
    );

    // SINSTS - Puissance apparente soutiree
    // 500 VA = 0.500 kVA
    std::string sinsts =
        makeValidTicLine("SINSTS\t00500\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            sinsts,
            LinkyParserMode::Standard,
            data
        )
    );

    // SINSTI - Puissance apparente injectee
    // 100 VA = 0.100 kVA
    std::string sinsti =
        makeValidTicLine("SINSTI\t00100\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            sinsti,
            LinkyParserMode::Standard,
            data
        )
    );

    // URMS1 - Tension phase 1
    std::string urms1 =
        makeValidTicLine("URMS1\t230\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            urms1,
            LinkyParserMode::Standard,
            data
        )
    );

    // IRMS1 - Courant phase 1
    std::string irms1 =
        makeValidTicLine("IRMS1\t002\t");

    TEST_ASSERT_TRUE(
        LinkyParser::parseLine(
            irms1,
            LinkyParserMode::Standard,
            data
        )
    );

    // Verification de la consommation
    TEST_ASSERT_TRUE(data.hasConsumption);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        1500.0f,
        data.consumptionKWh
    );

    // Verification de la production
    TEST_ASSERT_TRUE(data.hasProduction);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        250.0f,
        data.productionKWh
    );

    // Verification de la puissance apparente soutiree
    TEST_ASSERT_TRUE(
        data.hasApparentPowerConsumed
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        0.500f,
        data.apparentPowerConsumedKVA
    );

    // Verification de la puissance apparente injectee
    TEST_ASSERT_TRUE(
        data.hasApparentPowerProduced
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        0.100f,
        data.apparentPowerProducedKVA
    );

    // Les puissances actives ne sont pas fournies
    // par SINSTS et SINSTI.
    TEST_ASSERT_FALSE(
        data.hasPowerConsumed
    );

    TEST_ASSERT_FALSE(
        data.hasPowerProduced
    );

    // Verification de la tension
    TEST_ASSERT_TRUE(data.hasVoltage);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        230.0f,
        data.voltageV
    );

    // Verification du courant
    TEST_ASSERT_TRUE(data.hasCurrent);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        2.0f,
        data.currentA
    );
}

// ============================================================================
// TEST 3 - LIGNE INVALIDE
// ============================================================================

void test_linky_invalid_line() {

    LinkyParsedData data;

    bool result =
        LinkyParser::parseLine(
            "LIGNE_INVALIDE",
            LinkyParserMode::Historique,
            data
        );

    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// TEST 4 - ETIQUETTE INCONNUE
// ============================================================================

void test_linky_unknown_label() {

    LinkyParsedData data;

    std::string line =
        makeValidTicLine("UNKNOWN 12345 ");

    bool result =
        LinkyParser::parseLine(
            line,
            LinkyParserMode::Historique,
            data
        );

    // Une etiquette inconnue doit etre refusee.
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// TEST 5 - CHECKSUM VALIDE
// ============================================================================

void test_linky_valid_checksum() {

    std::string line =
        makeValidTicLine("ADCO 012345678901 ");

    bool result =
        LinkyParser::validateChecksum(line);

    TEST_ASSERT_TRUE(result);
}

// ============================================================================
// TEST 6 - CHECKSUM INCORRECT
// ============================================================================

void test_linky_invalid_checksum() {

    std::string line =
        makeValidTicLine("PAPP 00450 ");

    // Modifier la donnee apres calcul du checksum.
    size_t position = line.find("00450");

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        position
    );

    line[position + 4] = '1';

    // Le checksum doit etre refuse.
    bool result =
        LinkyParser::validateChecksum(line);

    TEST_ASSERT_FALSE(result);

    // Le parseur doit aussi refuser la ligne.
    LinkyParsedData data;

    bool parseResult =
        LinkyParser::parseLine(
            line,
            LinkyParserMode::Historique,
            data
        );

    TEST_ASSERT_FALSE(parseResult);
}

// ============================================================================
// TEST 7 - MODE STANDARD AVEC HORODATE
// ============================================================================

void test_linky_standard_timestamped() {

    LinkyParsedData data;

    // Format :
    // ETIQUETTE<TAB>HORODATE<TAB>DONNEE<TAB>CHECKSUM
    //
    // SINSTS : puissance apparente soutiree
    // 450 VA = 0.450 kVA

    std::string line =
        makeValidTicLine(
            "SINSTS\tH231007120000\t00450\t"
        );

    bool result =
        LinkyParser::parseLine(
            line,
            LinkyParserMode::Standard,
            data
        );

    // Le groupe doit etre accepte.
    TEST_ASSERT_TRUE(result);

    // L'horodate doit etre detectee.
    TEST_ASSERT_TRUE(data.hasTimestamp);

    TEST_ASSERT_EQUAL_STRING(
        "H231007120000",
        data.timestamp.c_str()
    );

    // La valeur doit etre 450 VA, pas l'horodate.
    TEST_ASSERT_TRUE(
        data.hasApparentPowerConsumed
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        0.450f,
        data.apparentPowerConsumedKVA
    );

    // Aucune puissance active en kW n'est fournie.
    TEST_ASSERT_FALSE(
        data.hasPowerConsumed
    );
}

// ============================================================================
// TEST 8 - ETIQUETTE STANDARD INCONNUE AVEC HORODATE
// ============================================================================

void test_linky_unknown_standard_label_does_not_modify_timestamp() {

    LinkyParsedData data;

    // Une horodate est deja presente.
    data.timestamp = "OLD_TIMESTAMP";
    data.hasTimestamp = true;

    // Groupe valide avec etiquette inconnue.
    std::string line =
        makeValidTicLine(
            "UNKNOWN\tH231007120000\t12345\t"
        );

    bool result =
        LinkyParser::parseLine(
            line,
            LinkyParserMode::Standard,
            data
        );

    // L'etiquette inconnue doit etre refusee.
    TEST_ASSERT_FALSE(result);

    // L'horodate existante doit etre conservee.
    TEST_ASSERT_TRUE(data.hasTimestamp);

    TEST_ASSERT_EQUAL_STRING(
        "OLD_TIMESTAMP",
        data.timestamp.c_str()
    );
}