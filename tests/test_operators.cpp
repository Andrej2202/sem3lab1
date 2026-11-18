#include "test_framework.hpp"
#include "NTree.hpp"
#include "CompareSelector.hpp"
#include <sstream>

void RunOperatorTests() {
    TEST_GROUP("Операторы");

    TEST("operator== для одинаковых деревьев")
        NTree<int> t1(3);
        t1.SetChildSelector(CompareSelector<int>());
        t1.Insert(10);
        t1.Insert(5);

        NTree<int> t2(3);
        t2.SetChildSelector(CompareSelector<int>());
        t2.Insert(10);
        t2.Insert(5);

        ASSERT_TRUE(t1 == t2);
    END_TEST

    TEST("operator!= для разных деревьев")
        NTree<int> t1(3);
        t1.SetChildSelector(CompareSelector<int>());
        t1.Insert(10);

        NTree<int> t2(3);
        t2.SetChildSelector(CompareSelector<int>());
        t2.Insert(20);

        ASSERT_TRUE(t1 != t2);
    END_TEST

    TEST("operator<<")
        NTree<int> tree(3);
        tree.SetChildSelector(CompareSelector<int>());
        tree.Insert(10);
        tree.Insert(5);

        stringstream ss;
        ss << tree;
        string out = ss.str();
        ASSERT_TRUE(out.find("10") != string::npos);
        ASSERT_TRUE(out.find("5") != string::npos);
    END_TEST
}