#include <unity.h>

// Fonctions obligatoires pour Unity
void setUp(void) {
    // Rien à préparer avant chaque test
}

void tearDown(void) {
    // Rien à nettoyer après chaque test
}

// Déclaration des 9 tests
void test_parse_valid_telegram(void);
void test_invalid_crc(void);
void test_missing_crc(void);
void test_wrong_start(void);
void test_empty_telegram(void);
void test_invalid_crc_characters(void);
void test_incomplete_telegram(void);
void test_incomplete_crc(void);
void test_oversized_telegram(void);
void test_extra_data_after_crc(void);
void test_successive_telegrams(void);
int main(int argc, char** argv)
{

    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_telegram);
    RUN_TEST(test_invalid_crc);
    RUN_TEST(test_missing_crc);
    RUN_TEST(test_wrong_start);
    RUN_TEST(test_empty_telegram);

    RUN_TEST(test_invalid_crc_characters);
    RUN_TEST(test_incomplete_telegram);
    RUN_TEST(test_incomplete_crc);
    RUN_TEST(test_oversized_telegram);
    RUN_TEST(test_extra_data_after_crc);
    RUN_TEST(test_successive_telegrams);

    return UNITY_END();
}