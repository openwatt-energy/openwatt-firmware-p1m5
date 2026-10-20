#include <unity.h>


// ============================================================================
// DECLARATION DES TESTS
// ============================================================================

void test_linky_historique();
void test_linky_standard();
void test_linky_invalid_line();
void test_linky_unknown_label();
void test_linky_valid_checksum();
void test_linky_invalid_checksum();

// Test 7 : groupe standard avec horodate
void test_linky_standard_timestamped();

// Test 8 : une étiquette standard inconnue
// ne doit pas modifier l'horodate existante
void test_linky_unknown_standard_label_does_not_modify_timestamp();


// ============================================================================
// UNITY
// ============================================================================

void setUp() {
}

void tearDown() {
}


// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {

    UNITY_BEGIN();

    RUN_TEST(test_linky_historique);
    RUN_TEST(test_linky_standard);
    RUN_TEST(test_linky_invalid_line);
    RUN_TEST(test_linky_unknown_label);
    RUN_TEST(test_linky_valid_checksum);
    RUN_TEST(test_linky_invalid_checksum);
    RUN_TEST(test_linky_standard_timestamped);
    RUN_TEST(
        test_linky_unknown_standard_label_does_not_modify_timestamp
    );

    return UNITY_END();
}