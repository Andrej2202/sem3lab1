#include "test_framework.hpp"
#include "NTree.hpp"
#include "CompareSelector.hpp"
#include "ModuloSelector.hpp"
#include "FirstFreeSelector.hpp"

void RunNTreeTests() {
    TEST_GROUP("NTree — базовые операции");

    TEST("Создание пустого дерева")
        NTree<int> tree(3);
        ASSERT_EQ(tree.GetSize(), 0);
        ASSERT_TRUE(tree.IsEmpty());
    END_TEST

    TEST("Вставка одного элемента")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_EQ(tree.GetSize(), 1);
        ASSERT_TRUE(!tree.IsEmpty());
    END_TEST

    TEST("Вставка нескольких элементов")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        tree.Insert(15);
        tree.Insert(7);
        ASSERT_EQ(tree.GetSize(), 5);
    END_TEST

    TEST("Поиск существующего элемента")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        auto* node = tree.Find(5);
        ASSERT_TRUE(node != nullptr);
        ASSERT_EQ(node->data, 5);
    END_TEST

    TEST("Поиск несуществующего элемента")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_TRUE(tree.Find(999) == nullptr);
    END_TEST

    TEST("Удаление листа")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        tree.Remove(5);
        ASSERT_EQ(tree.GetSize(), 2);
        ASSERT_TRUE(tree.Find(5) == nullptr);
    END_TEST

    TEST("Удаление корня")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        tree.Remove(10);
        ASSERT_EQ(tree.GetSize(), 2);
        ASSERT_TRUE(tree.Find(10) == nullptr);
    END_TEST

    TEST("Удаление несуществующего элемента")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_THROWS(tree.Remove(999));
    END_TEST

    TEST("Очистка дерева")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Clear();
        ASSERT_EQ(tree.GetSize(), 0);
        ASSERT_TRUE(tree.IsEmpty());
    END_TEST

    TEST_GROUP("NTree — явная вставка (без селектора)");

    TEST("Явная вставка создаёт корень без селектора")
        NTree<int> tree(3);
        tree.Insert(10, 0);
        ASSERT_EQ(tree.GetSize(), 1);
        ASSERT_TRUE(tree.Contains(10));
    END_TEST

    TEST("Явная вставка в ветку корня")
        NTree<int> tree(3);
        tree.Insert(10, 0);   // корень
        tree.Insert(5, 0);    // ветка 0 корня
        tree.Insert(20, 2);   // ветка 2 корня
        ASSERT_EQ(tree.GetSize(), 3);
        ASSERT_TRUE(tree.Contains(5));
        ASSERT_TRUE(tree.Contains(20));
    END_TEST

    TEST("Явная вставка по пути")
        NTree<int> tree(3);
        MutableArraySequence<int> path;
        tree.Insert(10, &path);   // пустой путь → корень
        path.Append(2);
        tree.Insert(20, &path);   // ветка 2 корня
        path.Append(1);
        tree.Insert(15, &path);   // ветка 2 → ветка 1
        ASSERT_EQ(tree.GetSize(), 3);
        ASSERT_TRUE(tree.Contains(15));
    END_TEST

    TEST("Явная вставка: занятая ветка — спуск к первой свободной")
        NTree<int> tree(3);
        tree.Insert(10, 0);
        tree.Insert(5, 0);    // ветка 0 корня
        tree.Insert(7, 0);    // ветка 0 занята → первый свободный потомок узла 5
        ASSERT_EQ(tree.GetSize(), 3);
        ASSERT_TRUE(tree.Contains(7));
    END_TEST

    TEST("Явная вставка: неверный индекс ветки")
        NTree<int> tree(3);
        ASSERT_THROWS(tree.Insert(10, 3));
        ASSERT_THROWS(tree.Insert(10, -1));
    END_TEST

    TEST("Явная вставка: null-путь")
        NTree<int> tree(3);
        ASSERT_THROWS(tree.Insert(10, static_cast<Sequence<int>*>(nullptr)));
    END_TEST

    TEST("Явная вставка: неверный индекс в пути")
        NTree<int> tree(3);
        MutableArraySequence<int> path;
        path.Append(5);
        ASSERT_THROWS(tree.Insert(10, &path));
    END_TEST

    TEST("Удаление без селектора запрещено")
        NTree<int> tree(3);
        tree.Insert(10, 0);
        ASSERT_THROWS(tree.Remove(10));
    END_TEST

    TEST("Явная и селекторная вставка сосуществуют")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);        // корень через селектор
        tree.Insert(5, 2);      // явно в ветку 2 (хотя 5 < 10)
        tree.Insert(20);        // через селектор: > 10 → ветка 2, занята → глубже
        ASSERT_EQ(tree.GetSize(), 3);
        ASSERT_TRUE(tree.Contains(5));
        ASSERT_TRUE(tree.Contains(20));
    END_TEST

    TEST_GROUP("NTree — ModuloSelector и FirstFreeSelector");

    TEST("ModuloSelector: значения расходятся по всем веткам (n=5)")
        NTree<int> tree(5);
        tree.SetChildSelector(ModuloSelector<int>());
        tree.Insert(10);
        tree.Insert(11);
        tree.Insert(12);
        tree.Insert(13);
        tree.Insert(14);
        tree.Insert(15);
        // Обход К123: корень, затем ветки 0..4.
        // 11..14 → ветки 1..4, 15 (разность 5) → ветка 0.
        MutableArraySequence<int> order;
        order.Append(0);
        for (int i = 1; i <= 5; ++i) order.Append(i);
        Sequence<int>* seq = tree.GetSequence(&order);
        const int expected[] = {10, 15, 11, 12, 13, 14};
        ASSERT_EQ(seq->GetLength(), 6u);
        for (int i = 0; i < 6; ++i) ASSERT_EQ(seq->Get(i), expected[i]);
        delete seq;
    END_TEST

    TEST("ModuloSelector: вставка/поиск/удаление 100 элементов (n=5)")
        NTree<int> tree(5);
        tree.SetChildSelector(ModuloSelector<int>());
        for (int i = 0; i < 100; ++i) tree.Insert((i * 37) % 101);
        ASSERT_EQ(tree.GetSize(), 100);
        for (int i = 0; i < 100; ++i) ASSERT_TRUE(tree.Contains((i * 37) % 101));
        for (int i = 0; i < 50; ++i) tree.Remove((i * 37) % 101);
        ASSERT_EQ(tree.GetSize(), 50);
        for (int i = 50; i < 100; ++i) ASSERT_TRUE(tree.Contains((i * 37) % 101));
    END_TEST

    TEST("FirstFreeSelector: структура дерева (обход К123)")
        NTree<int> tree(4);
        tree.SetChildSelector(FirstFreeSelector<int>());
        for (int i = 0; i < 9; ++i) tree.Insert(i);
        // Корень 0; ветки 0..3 → узлы 1,2,3,4; узел 1 заполняет свои ветки → 5,6,7,8.
        MutableArraySequence<int> order;
        order.Append(0);
        for (int i = 1; i <= 4; ++i) order.Append(i);
        Sequence<int>* seq = tree.GetSequence(&order);
        const int expected[] = {0, 1, 5, 6, 7, 8, 2, 3, 4};
        ASSERT_EQ(seq->GetLength(), 9u);
        for (int i = 0; i < 9; ++i) ASSERT_EQ(seq->Get(i), expected[i]);
        delete seq;
    END_TEST

    TEST("FirstFreeSelector: удаление корня с перевставкой потомков")
        NTree<int> tree(4);
        tree.SetChildSelector(FirstFreeSelector<int>());
        for (int i = 0; i < 9; ++i) tree.Insert(i);
        tree.Remove(0);
        ASSERT_EQ(tree.GetSize(), 8);
        for (int i = 1; i < 9; ++i) ASSERT_TRUE(tree.Contains(i));
        ASSERT_TRUE(tree.Find(0) == nullptr);
    END_TEST
}