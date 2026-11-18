#include "test_framework.hpp"
#include "CompareSelector.hpp"
#include "ModuloSelector.hpp"
#include "FirstFreeSelector.hpp"
#include "MutableArraySequence.hpp"

void RunSelectorTests() {
    TEST_GROUP("IChildSelector");

    TEST("CompareSelector: value < parent")
        CompareSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 5, 3), 0);
    END_TEST

    TEST("CompareSelector: value > parent")
        CompareSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 20, 3), 2);
    END_TEST

    TEST("CompareSelector: value == parent")
        CompareSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 10, 3), 1);
    END_TEST

    TEST("CompareSelector: клонирование")
        CompareSelector<int> sel;
        IChildSelector<int>* clone = sel.Clone();
        ASSERT_EQ(sel.SelectChild(10, 5, 3), clone->SelectChild(10, 5, 3));
        delete clone;
    END_TEST

    TEST("ModuloSelector: все n веток достижимы")
        ModuloSelector<int> sel;
        for (int i = 0; i < 5; ++i) {
            ASSERT_EQ(sel.SelectChild(10, 10 + i, 5), i);
        }
    END_TEST

    TEST("ModuloSelector: отрицательная разность нормализуется")
        ModuloSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 8, 5), 3);  // -2 mod 5 = 3
        ASSERT_EQ(sel.SelectChild(10, 9, 2), 1);  // -1 mod 2 = 1
    END_TEST

    TEST("ModuloSelector: дубликаты идут в ветку 0")
        ModuloSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 10, 4), 0);
    END_TEST

    TEST("ModuloSelector: детерминированность и клонирование")
        ModuloSelector<int> sel;
        IChildSelector<int>* clone = sel.Clone();
        ASSERT_EQ(sel.SelectChild(7, 123, 6), clone->SelectChild(7, 123, 6));
        delete clone;
    END_TEST

    TEST("FirstFreeSelector: первая свободная ветка")
        FirstFreeSelector<int> sel;
        MutableArraySequence<bool> occupied;
        occupied.Append(true);
        occupied.Append(false);
        occupied.Append(true);
        ASSERT_EQ(sel.SelectChild(10, 5, 3, &occupied), 1);
    END_TEST

    TEST("FirstFreeSelector: все ветки заняты — спуск в ветку 0")
        FirstFreeSelector<int> sel;
        MutableArraySequence<bool> occupied;
        for (int i = 0; i < 3; ++i) occupied.Append(true);
        ASSERT_EQ(sel.SelectChild(10, 5, 3, &occupied), 0);
    END_TEST

    TEST("FirstFreeSelector: без данных о занятости — ветка 0")
        FirstFreeSelector<int> sel;
        ASSERT_EQ(sel.SelectChild(10, 5, 3), 0);
        ASSERT_EQ(sel.SelectChild(10, 5, 3, nullptr), 0);
    END_TEST

    TEST("FirstFreeSelector: клонирование")
        FirstFreeSelector<int> sel;
        IChildSelector<int>* clone = sel.Clone();
        MutableArraySequence<bool> occupied;
        occupied.Append(true);
        occupied.Append(false);
        ASSERT_EQ(clone->SelectChild(10, 5, 2, &occupied), 1);
        delete clone;
    END_TEST
}