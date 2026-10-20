#ifndef P1_PARSER_H
#define P1_PARSER_H

#include <string>
#include <cstdint>

struct P1ParsedData {
    std::string equipmentId;
    std::string timestamp;

    uint8_t tariffIndicator = 0;

    float consumptionT1 = 0.0f;
    float consumptionT2 = 0.0f;

    float productionT1 = 0.0f;
    float productionT2 = 0.0f;

    float powerConsumed = 0.0f;
    float powerProduced = 0.0f;

    float avgDemand = 0.0f;
    float maxDemandMonth = 0.0f;
    float maxDemand13M = 0.0f;

    bool valid = false;
};

class P1Parser {
public:
    static P1ParsedData parse(const std::string& telegram);
    static uint16_t calculateCRC16(const char* data, size_t len);
    static bool validateCRC(const std::string& telegram);

private:
    static std::string extractStringValue(const std::string& line);
    static float extractFloatValue(const std::string& line);
    static int extractIntValue(const std::string& line);
};

#endif