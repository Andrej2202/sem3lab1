#include "test_framework.hpp"
#include "NTree.hpp"
#include "CompareSelector.hpp"

void RunSequenceRangeTests() {
    TEST_GROUP("GetSequence и GetRange");

    TEST("GetSequence (обход К123)")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);

        MutableArraySequence<int> order;
        order.Append(0);
        order.Append(1);
        order.Append(2);
        order.Append(3);

        Sequence<int>* result = tree.GetSequence(&order);
        ASSERT_TRUE(result->GetLength() > 0);
        delete result;
    END_TEST

    TEST("GetRange")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        tree.Insert(15);
        tree.Insert(7);

        Sequence<int>* result = tree.GetRange(7, 15);
        ASSERT_TRUE(result->GetLength() >= 3);
        delete result;
    END_TEST

    TEST("GetRange с некорректными границами")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_THROWS(tree.GetRange(20, 5));
    END_TEST
}