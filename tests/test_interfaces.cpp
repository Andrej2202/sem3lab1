#include "test_framework.hpp"
#include "NTree.hpp"
#include "CompareSelector.hpp"

void RunInterfaceTests() {
    TEST_GROUP("ICollection / IEnumerable");

    TEST("GetSize")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(20);
        ASSERT_EQ(tree.GetSize(), 2);
    END_TEST

    TEST("IsEmpty")
        NTree<int> tree(3);
        ASSERT_TRUE(tree.IsEmpty());
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        ASSERT_TRUE(!tree.IsEmpty());
    END_TEST

    TEST("Contains")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(20);
        ASSERT_TRUE(tree.Contains(10));
        ASSERT_TRUE(tree.Contains(20));
        ASSERT_TRUE(!tree.Contains(999));
    END_TEST

    TEST("Add (алиас Insert)")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Add(10);
        tree.Add(20);
        ASSERT_EQ(tree.GetSize(), 2);
    END_TEST

    TEST("GetEnumerator")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);
        tree.Insert(20);
        IEnumerator<int>* en = tree.GetEnumerator();
        int count = 0;
        while (en->MoveNext()) count++;
        ASSERT_EQ(count, 3);
        delete en;
    END_TEST

    TEST("MoveNext и GetCurrent")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        IEnumerator<int>* en = tree.GetEnumerator();
        ASSERT_TRUE(en->MoveNext());
        ASSERT_EQ(en->GetCurrent(), 10);
        ASSERT_TRUE(!en->MoveNext());
        delete en;
    END_TEST

    TEST("Reset")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(20);
        IEnumerator<int>* en = tree.GetEnumerator();
        en->MoveNext();
        en->Reset();
        ASSERT_TRUE(en->MoveNext());
        ASSERT_EQ(en->GetCurrent(), 10);
        delete en;
    END_TEST
}