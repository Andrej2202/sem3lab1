#include "test_framework.hpp"

void RunNTreeTests();
void RunSelectorTests();
void RunBuilderTests();
void RunInterfaceTests();
void RunOperatorTests();
void RunSequenceRangeTests();
void RunEdgeCaseTests();

int main() {
    cout << "========================================\n";
    cout << "Модульные тесты для n-арного дерева\n";
    cout << "========================================\n";

    RunNTreeTests();
    RunSelectorTests();
    RunBuilderTests();
    RunInterfaceTests();
    RunOperatorTests();
    RunSequenceRangeTests();
    RunEdgeCaseTests();

    cout << "\n========================================\n";
    cout << "Результаты тестирования:\n";
    cout << "  Пройдено: " << testsPassed << "\n";
    cout << "  Провалено: " << testsFailed << "\n";
    cout << "  Всего:    " << (testsPassed + testsFailed) << "\n";
    cout << "========================================\n";

    return testsFailed > 0 ? 1 : 0;
}