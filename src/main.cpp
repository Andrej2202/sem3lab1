#include <iostream>
#include <sstream>
#include <random>
#include <string>
#include <limits>

#include "NTreeBuilder.hpp"
#include "CompareSelector.hpp"
#include "ModuloSelector.hpp"
#include "FirstFreeSelector.hpp"

using namespace std;

static int ReadInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt << flush;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  Неверный ввод, введите целое число.\n";
    }
}

static void PrintSequence(const Sequence<int>& seq) {
    if (seq.GetLength() == 0) {
        cout << "(пусто)";
        return;
    }
    for (size_t i = 0; i < seq.GetLength(); ++i) {
        cout << seq.Get(i);
        if (i + 1 < seq.GetLength()) cout << " ";
    }
}

static const char* SelectorName(int kind) {
    switch (kind) {
        case 1:  return "CompareSelector";
        case 2:  return "ModuloSelector";
        case 3:  return "FirstFreeSelector";
        default: return "без селектора";
    }
}

static void PrintMenu(int n, size_t size, int selectorKind) {
    cout << "\n========== n-арное дерево (n=" << n
         << ", элементов=" << size
         << ", стратегия: " << SelectorName(selectorKind) << ") ==========\n";
    cout << "1.  Вставить один элемент\n";
    cout << "2.  Удалить элемент\n";
    cout << "3.  Найти элемент\n";
    cout << "4.  Размер / пусто\n";
    cout << "5.  Очистить дерево\n";
    cout << "6.  Показать дерево (обход К123)\n";
    cout << "7.  Обход по заданному порядку\n";
    cout << "8.  Диапазон [min, max]\n";
    cout << "9.  Итератор (DFS)\n";
    cout << "10. Ручной массовый ввод (числа через пробел)\n";
    cout << "11. Автоматический ввод (случайные числа)\n";
    cout << "12. Пересоздать дерево\n";
    cout << "13. Тест ICollection/IEnumerable и операторов\n";
    cout << "14. Явная вставка в ветку корня (без селектора)\n";
    cout << "15. Явная вставка по пути (без селектора)\n";
    cout << "0.  Выход\n";
    cout << "Выбор: ";
}

static void DoInsert(NTree<int>* tree) {
    int value = ReadInt("Введите значение: ");
    try {
        tree->Insert(value);
        cout << "Вставлено " << value << ".\n";
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoRemove(NTree<int>* tree) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    int value = ReadInt("Введите значение для удаления: ");
    try {
        tree->Remove(value);
        cout << "Удалено " << value << ".\n";
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoFind(NTree<int>* tree) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    int value = ReadInt("Искать значение: ");
    auto* node = tree->Find(value);
    cout << (node ? "Найдено: " + to_string(node->data) + "\n" : "Не найдено.\n");
}

static void DoSize(NTree<int>* tree) {
    cout << "Размер: " << tree->GetSize() << "\n";
    cout << "Пусто: " << (tree->IsEmpty() ? "да" : "нет") << "\n";
}

static void DoClear(NTree<int>* tree) {
    if (tree->IsEmpty()) {
        cout << "Дерево уже пусто.\n";
        return;
    }
    cout << "Вы уверены? (1 - да, 0 - нет): ";
    int confirm = ReadInt("");
    if (confirm == 1) {
        tree->Clear();
        cout << "Дерево очищено.\n";
    } else {
        cout << "Отмена.\n";
    }
}

static void DoPrintTree(NTree<int>* tree, int n) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    MutableArraySequence<int> order;
    order.Append(0);
    for (int i = 1; i <= n; ++i) order.Append(i);

    try {
        Sequence<int>* result = tree->GetSequence(&order);
        cout << "Обход К123: ";
        PrintSequence(*result);
        cout << "\n";
        delete result;
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoSequence(NTree<int>* tree, int n) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    cout << "Тип обхода:\n";
    cout << "  1 - К123 (прямой)\n";
    cout << "  2 - 123К (обратный)\n";
    cout << "  3 - 1К23 (симметричный, для n=3)\n";
    int type = ReadInt("Выбор: ");

    MutableArraySequence<int> order;
    if (type == 1) {
        order.Append(0);
        for (int i = 1; i <= n; ++i) order.Append(i);
    } else if (type == 2) {
        for (int i = 1; i <= n; ++i) order.Append(i);
        order.Append(0);
    } else if (type == 3) {
        order.Append(1);
        order.Append(0);
        for (int i = 2; i <= n; ++i) order.Append(i);
    } else {
        cout << "Неверный тип (допустимы 1, 2 или 3).\n";
        return;
    }

    try {
        Sequence<int>* result = tree->GetSequence(&order);
        cout << "Результат: ";
        PrintSequence(*result);
        cout << "\n";
        delete result;
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoRange(NTree<int>* tree) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    int lo = ReadInt("min: ");
    int hi = ReadInt("max: ");
    try {
        Sequence<int>* result = tree->GetRange(lo, hi);
        cout << "Элементы в [" << lo << ", " << hi << "]: ";
        PrintSequence(*result);
        cout << "\n";
        delete result;
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoIterator(NTree<int>* tree) {
    if (tree->IsEmpty()) {
        cout << "Дерево пусто.\n";
        return;
    }
    cout << "DFS-обход (старый итератор): ";
    int count = 0;
    for (auto it = tree->begin(); it != tree->end(); ++it) {
        cout << *it << " ";
        ++count;
    }
    cout << "\n(посещено узлов: " << count << ")\n";
}

static void DoBulkInsert(NTree<int>* tree) {
    cout << "Введите числа через пробел (Enter для завершения): ";
    string line;
    getline(cin, line);
    stringstream ss(line);
    int value;
    int ok = 0, fail = 0;
    while (ss >> value) {
        try { tree->Insert(value); ++ok; }
        catch (...) { ++fail; }
    }
    cout << "Вставлено: " << ok << ", ошибок: " << fail << "\n";
}

static void DoRandomInsert(NTree<int>* tree) {
    int count = ReadInt("Сколько случайных чисел вставить: ");
    int maxVal = ReadInt("Максимальное значение (от 0 до ...): ");
    int seed = ReadInt("Seed генератора (0 = случайный): ");

    if (seed == 0) {
        random_device rd;
        seed = rd();
    }
    mt19937 rng(seed);
    uniform_int_distribution<int> dist(0, maxVal);

    for (int i = 0; i < count; ++i) {
        tree->Insert(dist(rng));
    }
    cout << "Вставлено " << count << " случайных элементов (seed=" << seed << ").\n";
}

static void DoInsertIntoBranch(NTree<int>* tree, int n) {
    int value = ReadInt("Введите значение: ");
    int index = ReadInt("Номер ветки корня (0.." + to_string(n - 1) + "): ");
    try {
        tree->Insert(value, index);
        cout << "Вставлено " << value << " в ветку " << index << ".\n";
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoInsertByPath(NTree<int>* tree, int n) {
    int value = ReadInt("Введите значение: ");
    cout << "Путь из индексов веток (0.." << n - 1
         << ") через пробел (Enter — пустой путь): ";
    string line;
    getline(cin, line);
    stringstream ss(line);
    MutableArraySequence<int> path;
    int idx;
    while (ss >> idx) path.Append(idx);
    try {
        tree->Insert(value, &path);
        cout << "Вставлено " << value << " по пути длины " << path.GetLength() << ".\n";
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
    }
}

static void DoTestInterfaces(NTree<int>* tree) {
    cout << "\n--- Тест ICollection / IEnumerable и операторов ---\n";
    
    cout << "1. ICollection:\n";
    cout << "   GetSize() = " << tree->GetSize() << "\n";
    cout << "   IsEmpty() = " << (tree->IsEmpty() ? "true" : "false") << "\n";
    
    int val = ReadInt("   Введите значение для Contains(): ");
    cout << "   Contains(" << val << ") = " << (tree->Contains(val) ? "true" : "false") << "\n";

    cout << "\n2. IEnumerable (обход через IEnumerator):\n   ";
    IEnumerator<int>* enumerator = tree->GetEnumerator();
    while (enumerator->MoveNext()) {
        cout << enumerator->GetCurrent() << " ";
    }
    cout << "\n";
    delete enumerator;

    cout << "\n3. Операторы:\n";
    cout << "   operator<<: " << *tree << "\n";
    
    NTree<int> tree2(3);
    tree2.SetChildSelector(CompareSelector<int>());
    tree2.Insert(10);
    tree2.Insert(5);
    cout << "   Создали второе дерево {10, 5}.\n";
    cout << "   (*this == tree2) = " << (*tree == tree2 ? "true" : "false") << "\n";
    cout << "   (*this != tree2) = " << (*tree != tree2 ? "true" : "false") << "\n";
}

static int ReadSelectorKind() {
    cout << "Стратегия вставки:\n";
    cout << "  1 - CompareSelector: дерево поиска (только левая/правая ветки)\n";
    cout << "  2 - ModuloSelector: задействует все n веток (остаток разности)\n";
    cout << "  3 - FirstFreeSelector: первая свободная ветка\n";
    cout << "  0 - без селектора: только явная вставка (пункты 14-15)\n";
    int kind = ReadInt("Выбор стратегии: ");
    while (kind < 0 || kind > 3) {
        cout << "Допустимы 0, 1, 2 или 3.\n";
        kind = ReadInt("Выбор стратегии: ");
    }
    return kind;
}

static NTree<int>* CreateTree(int n, int selectorKind) {
    NTreeBuilder<int, MutableArraySequence<int>> builder(n);
    if (selectorKind == 1) {
        builder.SetSelector(CompareSelector<int>());
    } else if (selectorKind == 2) {
        builder.SetSelector(ModuloSelector<int>());
    } else if (selectorKind == 3) {
        builder.SetSelector(FirstFreeSelector<int>());
    }
    // kind == 0: селектор не устанавливаем — дерево работает через явную вставку
    return builder.Build();
}

int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");

    cout << "Лабораторная работа №1. n-арное дерево поиска.\n";
    cout << "Вариант 4. Реализация через IChildSelector + NTreeBuilder.\n\n";

    int n = ReadInt("Арность дерева n (n >= 2): ");
    while (n < 2) {
        cout << "Арность должна быть >= 2.\n";
        n = ReadInt("Арность дерева n: ");
    }

    int selectorKind = ReadSelectorKind();
    NTree<int>* tree = CreateTree(n, selectorKind);

    while (true) {
        PrintMenu(n, tree->GetSize(), selectorKind);
        int choice = ReadInt("");
        switch (choice) {
            case 1:  DoInsert(tree);       break;
            case 2:  DoRemove(tree);       break;
            case 3:  DoFind(tree);         break;
            case 4:  DoSize(tree);         break;
            case 5:  DoClear(tree);        break;
            case 6:  DoPrintTree(tree, n); break;
            case 7:  DoSequence(tree, n);  break;
            case 8:  DoRange(tree);        break;
            case 9:  DoIterator(tree);     break;
            case 10: DoBulkInsert(tree);   break;
            case 11: DoRandomInsert(tree); break;
            case 12:
                delete tree;
                n = ReadInt("Новая арность n: ");
                while (n < 2) {
                    cout << "Арность должна быть >= 2.\n";
                    n = ReadInt("Арность дерева n: ");
                }
                selectorKind = ReadSelectorKind();
                tree = CreateTree(n, selectorKind);
                cout << "Дерево пересоздано.\n";
                break;
            case 13: DoTestInterfaces(tree); break;
            case 14: DoInsertIntoBranch(tree, n); break;
            case 15: DoInsertByPath(tree, n); break;
            case 0:
                delete tree;
                cout << "Выход. До свидания!\n";
                return 0;
            default:
                cout << "Неверный пункт меню.\n";
        }
    }
}