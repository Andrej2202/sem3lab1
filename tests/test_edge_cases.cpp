#include "test_framework.hpp"
#include "NTree.hpp"
#include "CompareSelector.hpp"

void RunEdgeCaseTests() {
    TEST_GROUP("Граничные случаи");

    TEST("Дерево с одним элементом")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(42);
        ASSERT_EQ(tree.GetSize(), 1);
        ASSERT_TRUE(tree.Find(42) != nullptr);
        tree.Remove(42);
        ASSERT_EQ(tree.GetSize(), 0);
    END_TEST

    TEST("Вставка дубликатов")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(10);
        tree.Insert(10);
        ASSERT_EQ(tree.GetSize(), 3);
    END_TEST

    TEST("Установка селектора на непустое дерево")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_THROWS(tree.SetChildSelector(CompareSelector<int>()));
    END_TEST

    TEST("Вставка без селектора")
        NTree<int> tree(3);
        ASSERT_THROWS(tree.Insert(10));
    END_TEST

    TEST("Большое дерево (1000 элементов)")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        for (int i = 0; i < 1000; ++i) {
            tree.Insert(i);
        }
        ASSERT_EQ(tree.GetSize(), 1000);
        for (int i = 0; i < 1000; ++i) {
            ASSERT_TRUE(tree.Find(i) != nullptr);
        }
    END_TEST
}