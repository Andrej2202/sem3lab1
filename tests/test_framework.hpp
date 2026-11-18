#pragma once
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

inline int testsPassed = 0;
inline int testsFailed = 0;

#define TEST(name) \
    cout << "  Тест: " << name << "... "; \
    try {

#define END_TEST \
    cout << "✓ PASSED\n"; \
    testsPassed++; \
    } catch (const exception& e) { \
    cout << "✗ FAILED: " << e.what() << "\n"; \
    testsFailed++; \
    } catch (...) { \
    cout << "✗ FAILED: неизвестное исключение\n"; \
    testsFailed++; \
    }

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw runtime_error("Assertion failed: " #expr);

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw runtime_error("Assertion failed: " #a " != " #b);

#define ASSERT_THROWS(...) \
    { \
        bool caught = false; \
        try { __VA_ARGS__; } catch (...) { caught = true; } \
        if (!caught) throw runtime_error("Expected exception not thrown: " #__VA_ARGS__); \
    }

#define TEST_GROUP(name) cout << "\n=== " << name << " ===\n";