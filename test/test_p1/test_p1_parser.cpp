#include <unity.h>

#include "p1_parser.h"

#include <cstdio>
#include <string>

static std::string createValidTelegram() {

    std::string telegram =
        "/XMX5LGBBFG1009325509\r\n"
        "\r\n"
        "1-0:0.2.8(50)\r\n"
        "0-0:1.0.0(250116120000W)\r\n"
        "0-0:96.1.1(4530303531303035333030303035333137)\r\n"
        "1-0:1.8.1(001234.567*kWh)\r\n"
        "1-0:1.8.2(005678.901*kWh)\r\n"
        "1-0:2.8.1(000000.000*kWh)\r\n"
        "1-0:2.8.2(000000.000*kWh)\r\n"
        "0-0:96.14.0(0001)\r\n"
        "1-0:1.7.0(00.123*kW)\r\n"
        "1-0:2.7.0(00.000*kW)\r\n"
        "1-0:1.4.0(00.010*kW)\r\n"
        "1-0:1.6.0(250116170000W)(00.010*kW)\r\n"
        "0-0:98.1.0(00.020*kW)\r\n"
        "!";

    uint16_t crc = P1Parser::calculateCRC16(
        telegram.c_str(),
        telegram.length()
    );

    char crcBuffer[5];

    std::snprintf(
        crcBuffer,
        sizeof(crcBuffer),
        "%04X",
        crc
    );

    telegram += crcBuffer;
    telegram += "\r\n";

    return telegram;
}

// TEST 1 : Télégramme valide
void test_parse_valid_telegram(void) {

    std::string telegram = createValidTelegram();

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_TRUE(data.valid);

    TEST_ASSERT_EQUAL_STRING(
        "4530303531303035333030303035333137",
        data.equipmentId.c_str()
    );

    TEST_ASSERT_EQUAL_STRING(
        "250116120000W",
        data.timestamp.c_str()
    );

    TEST_ASSERT_EQUAL_UINT8(
        1,
        data.tariffIndicator
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        1234.567,
        data.consumptionT1
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        5678.901,
        data.consumptionT2
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.0,
        data.productionT1
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.0,
        data.productionT2
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.123,
        data.powerConsumed
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.0,
        data.powerProduced
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.010,
        data.avgDemand
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.010,
        data.maxDemandMonth
    );

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        0.020,
        data.maxDemand13M
    );
}

// TEST 2 : CRC incorrect
void test_invalid_crc(void) {

    std::string telegram = createValidTelegram();

    // Modifier une donnée sans recalculer le CRC
    size_t pos = telegram.find("001234.567");

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        pos
    );

    telegram[pos] = '9';

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 3 : CRC absent
void test_missing_crc(void) {

    std::string telegram =
        "/XMX5\r\n"
        "1-0:1.8.1(001234.567*kWh)\r\n";

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 4 : Mauvais début
void test_wrong_start(void) {

    std::string telegram =
        "XMX5\r\n"
        "1-0:1.8.1(001234.567*kWh)\r\n"
        "!1234\r\n";

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 5 : Télégramme vide
void test_empty_telegram(void) {

    P1ParsedData data =
        P1Parser::parse("");

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 6 : CRC contenant des caractères invalides
void test_invalid_crc_characters(void) {

    std::string telegram = createValidTelegram();

    size_t pos = telegram.rfind('!');

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        pos
    );

    telegram.replace(pos + 1, 4, "ZZZZ");

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 7 : Télégramme incomplet
void test_incomplete_telegram(void) {

    std::string telegram = createValidTelegram();

    size_t pos = telegram.rfind('!');

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        pos
    );

    telegram.erase(pos);

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 8 : CRC incomplet
void test_incomplete_crc(void) {

    std::string telegram = createValidTelegram();

    size_t pos = telegram.rfind('!');

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        pos
    );

    // Conserver seulement deux caractères CRC
    telegram.erase(pos + 3);

    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}

// TEST 9 : Télégramme trop long
void test_oversized_telegram(void) {

    std::string telegram = createValidTelegram();

    size_t pos = telegram.rfind('!');

    TEST_ASSERT_NOT_EQUAL(
        std::string::npos,
        pos
    );

    // Ajouter 2500 caractères avant le CRC
    telegram.insert(pos, std::string(2500, 'A'));

    // Recalculer le CRC
    size_t crcPos = telegram.rfind('!');

    uint16_t crc = P1Parser::calculateCRC16(
        telegram.c_str(),
        crcPos + 1
    );

    char crcBuffer[5];

    std::snprintf(
        crcBuffer,
        sizeof(crcBuffer),
        "%04X",
        crc
    );

    telegram.replace(crcPos + 1, 4, crcBuffer);

    // Vérifier que le CRC est correct
    TEST_ASSERT_TRUE(
        P1Parser::validateCRC(telegram)
    );

    // Vérifier que le télégramme dépasse 2048 octets
    TEST_ASSERT_TRUE(
        telegram.size() > 2048
    );

    // Le parseur doit refuser ce télégramme
    P1ParsedData data =
        P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}
    // TEST 10 : données supplémentaires après le CRC
void test_extra_data_after_crc(void) {

    std::string telegram = createValidTelegram();

    // Ajouter des données après le CRC
    telegram += "DONNEES_INATTENDUES";

    P1ParsedData data = P1Parser::parse(telegram);

    TEST_ASSERT_FALSE(data.valid);
}
// TEST 11 : deux télégrammes successifs
void test_successive_telegrams(void) {

    // Premier télégramme
    std::string telegram1 = createValidTelegram();

    P1ParsedData data1 = P1Parser::parse(telegram1);

    TEST_ASSERT_TRUE(data1.valid);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        1234.567,
        data1.consumptionT1
    );

    // Deuxième télégramme avec une consommation différente
    std::string telegram2 = createValidTelegram();

    size_t pos = telegram2.find("001234.567");

    TEST_ASSERT_NOT_EQUAL(std::string::npos, pos);

    telegram2.replace(pos, 10, "002345.678");

    // Recalculer le CRC du deuxième télégramme
    size_t crcPos = telegram2.rfind('!');

    TEST_ASSERT_NOT_EQUAL(std::string::npos, crcPos);

    uint16_t crc = P1Parser::calculateCRC16(
        telegram2.c_str(),
        crcPos + 1
    );

    char crcBuffer[5];

    std::snprintf(
        crcBuffer,
        sizeof(crcBuffer),
        "%04X",
        crc
    );

    telegram2.replace(crcPos + 1, 4, crcBuffer);

    // Analyser le deuxième télégramme
    P1ParsedData data2 = P1Parser::parse(telegram2);

    TEST_ASSERT_TRUE(data2.valid);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        2345.678,
        data2.consumptionT1
    );

    // Vérifier que le premier résultat n'a pas changé
    TEST_ASSERT_FLOAT_WITHIN(
        0.001,
        1234.567,
        data1.consumptionT1
    );
}