#include "test_framework.hpp"
#include "NTreeBuilder.hpp"
#include "CompareSelector.hpp"

void RunBuilderTests() {
    TEST_GROUP("NTreeBuilder");

    TEST("Создание дерева через билдер")
        NTreeBuilder<int, MutableArraySequence<int>> builder(3);
        builder.SetSelector(CompareSelector<int>());
        builder.Add(10).Add(5).Add(20);
        NTree<int>* tree = builder.Build();
        ASSERT_EQ(tree->GetSize(), 3);
        ASSERT_TRUE(tree->Find(10) != nullptr);
        ASSERT_TRUE(tree->Find(5) != nullptr);
        ASSERT_TRUE(tree->Find(20) != nullptr);
        delete tree;
    END_TEST

    TEST("Добавление последовательности")
        NTreeBuilder<int, MutableArraySequence<int>> builder(3);
        builder.SetSelector(CompareSelector<int>());
        MutableArraySequence<int> seq;
        seq.Append(100);
        seq.Append(200);
        seq.Append(300);
        builder.AddSequence(&seq);
        NTree<int>* tree = builder.Build();
        ASSERT_EQ(tree->GetSize(), 3);
        delete tree;
    END_TEST

    TEST("Очистка билдера")
        NTreeBuilder<int, MutableArraySequence<int>> builder(3);
        builder.SetSelector(CompareSelector<int>());
        builder.Add(10).Add(20);
        builder.Clear();
        NTree<int>* tree = builder.Build();
        ASSERT_EQ(tree->GetSize(), 0);
        delete tree;
    END_TEST

    TEST("Некорректная арность")
        ASSERT_THROWS(
            []() {
                NTreeBuilder<int, MutableArraySequence<int>> b(0);
            }()
        );
    END_TEST
}