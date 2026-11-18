# Архитектура лабораторной работы №1 — n-арное дерево (Вариант 4, C++20)

Документ описывает, как устроена программа: классы, их связи, потоки данных
при вставке/удалении/обходе и управление памятью.

---

## 1. Карта файлов

```
lab1/
├── include/
│   ├── Interfaces.hpp          ICollection<T>, IEnumerable<T>, IEnumerator<T>
│   ├── IChildSelector.hpp      интерфейс стратегии выбора ветки (+ занятость)
│   ├── CompareSelector.hpp     стратегия: < → 0, > → n-1, = → n/2
│   ├── ModuloSelector.hpp      стратегия: (value − parent) mod n — все ветки
│   ├── FirstFreeSelector.hpp   стратегия: первая свободная ветка 0..n-1
│   ├── IBuilder.hpp            интерфейс строителя ITreeBuilder<T>
│   ├── NTreeBuilder.hpp        строитель дерева (буфер + селектор + Build)
│   ├── NTree.hpp               дерево: объявления, Node, итераторы
│   ├── NTree.tpp               дерево: реализация (подключается в конце .hpp)
│   └── base/                   базовые контейнеры и исключения
│       ├── Sequence.hpp / ArraySequence / MutableArraySequence / ListSequence
│       ├── DynamicArray.hpp / LinkedList.hpp / Iterator.hpp
│       └── Exceptions.hpp      InvalidArgumentException, IndexOutOfRangeException, ...
├── src/main.cpp                консольное меню (демо всех возможностей)
├── src/main_qt.cpp             графическое меню (Qt6 Widgets, аналог консольного)
├── tests/                      56 юнит-тестов (свой мини-фреймворк)
└── CMakeLists.txt              цели: lab1 (меню), tests, lab1_gui (если найден Qt6)
```

---

## 2. Диаграмма классов

```
 Interfaces.hpp                        IChildSelector.hpp
┌─────────────────────┐               ┌──────────────────────────────────────┐
│ ICollection<T>      │               │ IChildSelector<T>                    │
│   GetSize()         │               │   SelectChild(parent, value, n) = 0  │
│   IsEmpty()         │               │   SelectChild(parent, value, n,      │
│   Contains(value)   │               │               occupied)              │
│   Add(value)        │               │     └─ по умолчанию → 3-аргументная  │
├─────────────────────┤               │   Clone() = 0                        │
│ IEnumerable<T>      │               └──────────────────┬───────────────────┘
│   GetEnumerator()   │                                  │ наследуют
└──────────┬──────────┘               ┌──────────────────┼─────────────────────┐
           │ реализует                │                  │                     │
           ▼                 CompareSelector<T>  ModuloSelector<T>   FirstFreeSelector<T>
      ┌─────────┐            ветка по знаку     ветка = остаток     первая свободная
      │ NTree<T>│            сравнения:        разности:           ветка (нужна
      └────┬────┘            < → 0             (v − p) mod n       информация о
           │                 > → n−1           (все n веток!)      занятости)
           │                 = → n/2
           │
           │ владеет (new/delete)
           ├──────────────► Node { T data; Node** children; int n; }
           │                  └─ children — массив из n указателей (nullptr = свободно)
           │
           └──────────────► IChildSelector<T>* childSelector_
                              (получен через Clone(), дерево удаляет его в деструкторе)

      ┌──────────────────────────────┐
      │ NTreeBuilder<T, Container>   │  concept ContainerLike: Append/Get/GetLength
      │  (ITreeBuilder<T>)           │
      │   SetSelector(sel) → Clone() │──── Build() ────► new NTree<T>(n)
      │   Add / AddSequence / Clear  │                   + SetChildSelector(*selector_)
      │   Build / BuildBalanced      │                   + Insert каждого элемента буфера
      └──────────────────────────────┘
```

Ключевая идея — **паттерн «Стратегия»**: `NTree` не знает, как выбирается ветка.
Он спрашивает `childSelector_->SelectChild(...)`. Меняя селектор, меняем форму
дерева, не трогая код дерева.

---

## 3. Интерфейс селектора и занятость веток

```cpp
// 3-аргументная: выбор только по значениям (обязательная)
virtual int SelectChild(const T& parentValue, const T& value, int n) const = 0;

// 4-аргументная: выбор с учётом занятости веток узла
virtual int SelectChild(const T& parentValue, const T& value, int n,
                        const Sequence<bool>* occupied) const {
    return SelectChild(parentValue, value, n);   // делегация по умолчанию
}
```

- `occupied[i] == true` — ветка `i` узла уже занята.
- `CompareSelector` и `ModuloSelector` переопределяют только 3-аргументную
  версию → занятость им не нужна, работает делегация.
- `FirstFreeSelector` переопределяет 4-аргументную: ищет первый `false`
  в `occupied`; если все ветки заняты — возвращает 0 (спуск глубже).
- `Clone()` — виртуальная копия: дерево и строитель хранят собственные копии
  селекторов, переданные извне объекты им не принадлежат.

### Как стратегии заполняют ветки (n = 5, вставляем 50, 51, 52, 53, 54, 55)

**CompareSelector** — только крайние ветки (исходная проблема лабы):
все числа больше родителя → всегда ветка 4 → вырожденная цепочка:

```
[50]
  └─ ветка 4: [51]
                └─ ветка 4: [52]
                              └─ ветка 4: [53] → [54] → [55]
```

**ModuloSelector** — ветка = `(value − parent) mod n` (отрицательный остаток
нормализуется: `−1 mod 5 = 4`). Разности 1,2,3,4,5 → ветки 1,2,3,4,0:

```
                  [50]
      ┌──────┬──────┼──────┬──────┐
   ветка0  ветка1  ветка2  ветка3  ветка4
    [55]    [51]    [52]    [53]    [54]      ← заняты ВСЕ 5 веток
```

**FirstFreeSelector** (n = 4, вставляем 0..8) — первая свободная ветка,
при всех занятых — спуск в ветку 0:

```
                     [0]
       ┌──────┬──────┼──────┐
      [1]    [2]    [3]    [4]        ← ветки 0..3 корня
   ┌───┼───┬───┐
  [5] [6] [7] [8]                    ← все ветки узла 1 заняты → глубже
```

---

## 4. Вставка через селектор: `Insert(value)`

```
Insert(value)
   │
   ├─ childSelector_ == nullptr? ──да──► InvalidArgumentException
   ▼
InsertRecursive(root_, value)          ← рекурсия по ссылке Node*&
   │
   ├─ слот пуст (node == nullptr)? ──да──► node = new Node(value, n_); size_++
   │                                        (так создаётся и корень)
   ▼
occupied = [children[0] != nullptr, ..., children[n-1] != nullptr]
   │        (MutableArraySequence<bool>, ровно n элементов)
   ▼
childIndex = childSelector_->SelectChild(node->data, value, n_, &occupied)
   │
   ├─ индекс вне [0, n)? ──да──► InvalidArgumentException
   ▼
InsertRecursive(node->children[childIndex], value)   ← спуск в выбранную ветку
```

Важно: вставка **не останавливается на равном значении** — дубликаты уходят
вглубь по правилу селектора (Compare: ветка `n/2`, Modulo: ветка 0, т.к.
разность 0). Поэтому все копии одного значения лежат на одном пути цепочкой.

---

## 5. Явная вставка без селектора (задача 5)

Дерево может работать как обычное n-арное дерево: структуру задаёт пользователь.

```cpp
tree.Insert(10, 0);        // в ветку 0 корня (если корня нет — станет корнем)
tree.Insert(15, &path);    // path = {2, 1} — по пути из индексов веток
```

```
Insert(value, childIndex)                Insert(value, path)
   │ проверка 0 <= childIndex < n           │ path == nullptr → InvalidArgumentException
   │ иначе IndexOutOfRangeException         │ проверка каждого индекса пути
   ▼                                        ▼
   строит path = [childIndex] ──────► InsertExplicit(root_, value, path, 0)
                                         │
                                         ├─ слот пуст → new Node(value), size_++
                                         │
                                         ├─ путь не исчерпан → спуск по path[pathPos]
                                         │   (занятый слот просто проходим сквозь)
                                         │
                                         └─ путь исчерпан, узел занят
                                             ▼
                                       InsertFirstFree(node, value)
                                         ├─ первая пустая ветка → new Node там
                                         └─ все заняты → спуск в ветку 0, повтор
```

Пример (n = 3): `Insert(10, 0)`, `Insert(20, path[2])`, `Insert(15, path[2,1])`:

```
         [10]
     ┌────┼────┐
  ветка0 ветка1 ветка2
    ∅      ∅     [20]
              ┌───┼───┐
            ∅  [15]   ∅      ← путь [2,1]: ветка 2 занята → прошли в узел 20,
                               её ветка 1 свободна → значение там
```

Селектор для этих перегрузок **не нужен**. Явная и селекторная вставки
сосуществуют: явная ставит узел куда скажут, селекторная продолжает работать
от корня по своим правилам.

---

## 6. Удаление: `Remove(value)`

```
Remove(value)
   │
   ├─ childSelector_ == nullptr? ──да──► InvalidArgumentException
   ▼                                     (селектор нужен для перевставки потомков)
RemoveRecursive(root_, value) — ПОИСК ПО ВСЕМ ВЕТКАМ (DFS)
   │
   ├─ node == nullptr → false
   ├─ node->data != value → рекурсия в children[0..n-1] по очереди
   │
   └─ node->data == value:
       1. CollectValues(children[0..n-1]) → список ВСЕХ значений потомков
       2. DestroyRecursive(node); node = nullptr        ← узел и поддерево удалены
       3. size_ -= (потомки + 1)
       4. каждое значение потомка: InsertRecursive(root_, v)  ← перевставка
          через селектор от корня (инвариант дерева сохраняется)
   │
   └─ не найдено нигде → InvalidArgumentException("Element not found")
```

**Почему поиск по всем веткам, а не навигация через селектор?**
`FirstFreeSelector` выбирает ветку по занятости на момент вставки — при
удалении занятость другая, путь не воспроизвести. DFS находит узел при любой
стратегии. Для детерминированных селекторов (Compare, Modulo) результат тот же:
все копии значения лежат на одном пути, и DFS preorder находит самую близкую
к корню — ровно ту, которую нашла бы навигация. Цена: O(size) вместо O(height).

Ни одно поддерево не теряется: значения потомков собираются **до** удаления
узла (это закрывает задачу 2 из AGENTS.md про средние ветки).

---

## 7. Обходы, поиск, диапазон

### GetSequence(order) — обход по заданному порядку
`order` — последовательность длины **n+1**: `0` = вывести значение узла,
`1..n` = обойти ветку `idx−1`. Индекс вне диапазона → InvalidArgumentException.

```
К123 (прямой):      [0, 1, 2, ..., n]     узел → ветка0 → ветка1 → ...
123К (обратный):    [1, 2, ..., n, 0]     ветки → узел
1К23 (симметр.):    [1, 0, 2, ..., n]     ветка0 → узел → остальные
```

### TreeEnumerator / Iterator — DFS preorder через стек
```
Reset: стек = [root]
MoveNext / ++:  снять вершину → положить её детей в обратном порядке
                (чтобы ветка 0 обрабатывалась первой)
GetCurrent / *: значение вершины стека
```
`GetEnumerator()` возвращает `IEnumerator<T>*` (IEnumerable), `begin()/end()` —
старый `Iterator` для range-based циклов. Оба владеют своим стеком
(`MutableArraySequence<Node*>`) и удаляют его в деструкторе.

### Find / Contains / GetRange — полный обход
Не зависят от стратегии селектора:
- `Find(value)` — DFS preorder, возвращает `Node*` или nullptr;
- `GetRange(min, max)` — обход всего дерева, сбор значений в `[min, max]`
  (min > max → InvalidArgumentException). Поэтому они корректны даже когда
  дерево не упорядочено (Modulo/FirstFree).

---

## 8. Строитель: NTreeBuilder

```
NTreeBuilder<int, MutableArraySequence<int>> builder(n);
builder.SetSelector(ModuloSelector<int>());   // хранит Clone() селектора
builder.Add(50).Add(51);                      // буфер элементов (ContainerLike)
NTree<int>* tree = builder.Build();           // new NTree(n) + селектор + Insert всех
```

- `concept ContainerLike` проверяет у контейнера `Append/Get/GetLength`.
- `Build()` вставляет элементы буфера **в порядке добавления**.
- `BuildBalanced()` пока просто вызывает `Build()` (задача 6 AGENTS.md —
  сортировка + медианная вставка — ещё не реализована).
- Владеет буфером и собственной копией селектора; удаляет их в деструкторе.
  Построенное дерево передаётся вызывающему (он и удаляет).

---

## 9. Владение памятью (кто что удаляет)

| Объект | Кто владеет | Где удаляется |
|---|---|---|
| Узлы дерева (`Node`, массивы `children`) | `NTree` | `DestroyRecursive` в `~NTree`/`Clear`/`Remove` |
| `childSelector_` (копия через `Clone`) | `NTree` | `~NTree`, замена в `SetChildSelector` |
| Буфер элементов + копия селектора | `NTreeBuilder` | `~NTreeBuilder`, `Clear` |
| Стек обхода | `TreeEnumerator` / `Iterator` | их деструкторы |
| Результат `GetSequence`/`GetRange` | вызывающий код | `delete` после использования |
| Дерево из `builder.Build()` | вызывающий код | `delete` (в меню — при пересоздании/выходе) |

Правила:
- `NTree` некопируем (`copy ctor` и `operator=` удалены) — двойное владение узлами запрещено.
- Селекторы передаются **по константной ссылке**, внутри делается `Clone()` —
  исходный объект остаётся у вызывающего (задача 1 AGENTS.md, утечек нет).
- `SetChildSelector` разрешён только пустому дереву (иначе InvalidArgumentException).
- Проверка: юнит-тесты + сценарии меню под AddressSanitizer/UBSan — утечек нет.

---

## 10. Исключения (base/Exceptions.hpp)

| Исключение | Где бросается |
|---|---|
| `InvalidArgumentException` | `Insert`/`Remove` без селектора; null-путь/порядок; `min > max`; неверный индекс от селектора; элемент не найден при `Remove`; `n <= 0`; смена селектора непустого дерева |
| `IndexOutOfRangeException` | неверный `childIndex`/индекс пути при явной вставке; неверный индекс порядка в `GetSequence`; `GetCurrent` без текущего; `operator*`/`++` пустого итератора |

---

## 11. Консольное меню (src/main.cpp)

При старте и пересоздании (пункт 12): арность `n >= 2`, затем выбор стратегии:
```
1 — CompareSelector    (упорядоченное дерево поиска, только крайние ветки)
2 — ModuloSelector     (все n веток, остаток разности)
3 — FirstFreeSelector  (первая свободная ветка)
0 — без селектора      (только явная вставка, пункты 14–15)
```
Пункты: 1 вставка · 2 удаление · 3 поиск · 4 размер · 5 очистка · 6 обход К123 ·
7 обход по порядку · 8 диапазон · 9 DFS-итератор · 10 массовый ввод ·
11 случайные числа · 12 пересоздать · 13 тест интерфейсов/операторов ·
**14 явная вставка в ветку корня · 15 явная вставка по пути** · 0 выход.

---

## 12. Сборка и проверка

```bash
cmake -S . -B build && cmake --build build -j
./build/tests        # 56 юнит-тестов
./build/lab1         # интерактивное меню
```

Группы тестов: базовые операции NTree · явная вставка без селектора ·
ModuloSelector/FirstFreeSelector (включая проверку структуры через обход К123) ·
селекторы · строитель · ICollection/IEnumerable · операторы · GetSequence/GetRange ·
граничные случаи (дубликаты, 1000 элементов, удаление без селектора и т.п.).

**Нюанс окружения:** Valgrind в системе зависает даже на `/bin/true`, а пакет
`libasan` не установлен. Обход: `dnf download libasan libubsan` (без root),
распаковка rpm в `/tmp/opencode/sanlibs`, сборка с флагами
`-fsanitize=address,undefined -L/tmp/opencode/sanlibs/usr/lib64
-Wl,-rpath,/tmp/opencode/sanlibs/usr/lib64`.
Тесты и сценарии меню прогнаны под ASan+UBSan — ошибок и утечек нет.

---

## 13. Графическое меню (Qt6): src/main_qt.cpp

Цель `lab1_gui` создаётся в CMake только если `find_package(Qt6 COMPONENTS
Widgets)` нашёл Qt; консольные цели от Qt не зависят.

### Структура окна

```
MainWindow (QMainWindow)
├── левая панель (QScrollArea): группы кнопок/полей — все пункты меню 1–15
│     Дерево: пересоздать (12), очистить (5), размер/пусто (4)
│     Значение: вставить (1), удалить (2), найти (3)
│     Явная вставка: ветка корня (14), путь (15)
│     Массовый ввод: числа через пробел (10), случайные count/max/seed (11)
│     Обходы: К123/123К/1К23/свой порядок (6–7), DFS-итератор (9),
│             диапазон [min,max] (8), тест интерфейсов (13)
├── правая часть (QSplitter): TreeView (QGraphicsView) + журнал (QTextEdit)
└── статус-бар: n · элементов · стратегия
```

При старте и пересоздании — модальный диалог арности и стратегии (аналог
стартовых вопросов консоли). Логика операций — те же вызовы `NTree`/
`NTreeBuilder`, что и в консольном `main.cpp`, обёрнутые в try/catch с
выводом в журнал.

### Визуализация дерева

- `NTree::GetRoot()` — единственный аксессор, добавленный в `NTree.hpp`
  ради отрисовки (возвращает приватный `Node*`, вызывающий работает через
  `auto`, как и с `Find()`). Поведение дерева не меняется.
- Раскладка: `SubtreeUnits` — ширина поддерева в "единицах" (лист = 1,
  узел = сумма ширин детей); `DrawNode` рекурсивно рисует детей слева
  направо ( offsets в пикселях = единицы × `kUnitW`), родитель — по центру
  над детьми. Рёбра подписаны номерами веток.
- Подсветка: найденный `Find` узел — оранжевый (+ центрирование вида),
  узлы диапазона `GetRange` — зелёные; корень — темно-синий.
- Зум колесом мыши, кнопки «Вписать / + / −», чекбокс авто-вписывания;
  `TreeView::resizeEvent` перевписывает сцену после получения viewport'ом
  реального размера (иначе первый fit считается по нулевому размеру).
- Сцена перестраивается целиком после каждой мутации (`RefreshScene`).

### Владение памятью в GUI

`tree_` удаляется в `~MainWindow`; `QGraphicsScene` и все виджеты владеются
QObject-родителями; результаты `GetSequence`/`GetRange` и `IEnumerator`
удаляются сразу после использования (правила §9 без изменений).

### Проверка без дисплея

```bash
QT_QPA_PLATFORM=offscreen ./build/lab1_gui --selftest
```

`--selftest` прогоняет все операции (вставки обоих видов, массовый/случайный
ввод, поиск, диапазон, все обходы, итератор, тест интерфейсов, удаление,
очистку, пересоздание, режим без селектора), проверяет размеры и ожидаемые
исключения, дублирует журнал в stdout и сохраняет снимок окна в
`/tmp/lab1_gui_selftest.png`. Код возврата 0 — все шаги прошли.

### Нюанс окружения (Qt без root)

В системе установлен только runtime Qt6 (`qt6-qtbase`, `qt6-qtbase-gui`),
dev-пакета нет, sudo требует пароль. Обход в духе §12:

```bash
dnf download qt6-qtbase-devel                 # без root
mkdir -p /tmp/opencode/qt6-devel && cd /tmp/opencode/qt6-devel
rpm2cpio qt6-qtbase-devel-*.x86_64.rpm | cpio -idm
# достроить ссылки на системные runtime-библиотеки и плагины:
cd usr/lib64
for f in /usr/lib64/libQt6*.so.6*; do b=$(basename "$f"); [ -e "$b" ] || ln -s "$f" "$b"; done
for f in libQt6*.so; do [ -e "$f" ] || ln -s "/usr/lib64/$(readlink "$f")" "$f"; done
mkdir -p qt6/bin && ln -s /usr/lib64/qt6/bin/qtpaths qt6/bin/qtpaths
ln -s /usr/lib64/qt6/plugins qt6/plugins
# конфигурация и сборка:
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/opencode/qt6-devel/usr
cmake --build build -j
```

cmake-конфиги Qt6 релокабельны (пути считаются относительно расположения
конфигов), поэтому распакованный префикс работает без установки в систему.