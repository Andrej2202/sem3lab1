// Графическое меню лабораторной работы №1 (Qt6 Widgets).
// Полный аналог консольного src/main.cpp: те же операции (пункты 1–15),
// плюс визуализация дерева, подсветка найденных узлов и диапазона.
//
// Сборка: cmake -S . -B build -DCMAKE_PREFIX_PATH=<префикс Qt6>
//         cmake --build build --target lab1_gui
// Проверка без дисплея: QT_QPA_PLATFORM=offscreen ./build/lab1_gui --selftest

#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QTextEdit>
#include <QTextDocument>
#include <QDir>
#include <QStatusBar>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <QVector>
#include <QPair>

#include <sstream>
#include <random>
#include <limits>
#include <algorithm>

#include "NTreeBuilder.hpp"
#include "CompareSelector.hpp"
#include "ModuloSelector.hpp"
#include "FirstFreeSelector.hpp"

// ---------------------------------------------------------------------------
// Вспомогательные функции (как в консольном main.cpp)
// ---------------------------------------------------------------------------

static const char* SelectorName(int kind) {
    switch (kind) {
        case 1:  return "CompareSelector";
        case 2:  return "ModuloSelector";
        case 3:  return "FirstFreeSelector";
        default: return "без селектора";
    }
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

// Диалог параметров дерева (аналог стартовых вопросов консоли и пункта 12).
static bool AskTreeParams(QWidget* parent, int& n, int& kind) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Параметры дерева");
    auto* layout = new QVBoxLayout(&dlg);

    auto* form = new QFormLayout();
    auto* nSpin = new QSpinBox(&dlg);
    nSpin->setRange(2, 64);
    nSpin->setValue(n);
    form->addRow("Арность n (>= 2):", nSpin);

    auto* kindCombo = new QComboBox(&dlg);
    kindCombo->addItem("1 — CompareSelector (дерево поиска, крайние ветки)");
    kindCombo->addItem("2 — ModuloSelector (все n веток, остаток разности)");
    kindCombo->addItem("3 — FirstFreeSelector (первая свободная ветка)");
    kindCombo->addItem("0 — без селектора (только явная вставка)");
    // индекс в комбобоксе -> kind: 0,1,2,3
    static const int kinds[4] = {1, 2, 3, 0};
    for (int i = 0; i < 4; ++i) {
        if (kinds[i] == kind) { kindCombo->setCurrentIndex(i); break; }
    }
    form->addRow("Стратегия вставки:", kindCombo);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) return false;
    n = nSpin->value();
    kind = kinds[kindCombo->currentIndex()];
    return true;
}

// ---------------------------------------------------------------------------
// Визуализация дерева
// ---------------------------------------------------------------------------

static constexpr double kNodeW  = 56;   // ширина узла
static constexpr double kNodeH  = 32;   // высота узла
static constexpr double kUnitW  = 72;   // ширина "единицы" раскладки (узел + зазор)
static constexpr double kLevelH = 80;   // расстояние между уровнями

// Ширина поддерева в "единицах": лист = 1, узел = сумма ширин детей (минимум 1).
template <class Node>
static double SubtreeUnits(const Node* node) {
    if (!node) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < node->n; ++i) sum += SubtreeUnits(node->children[i]);
    return std::max(sum, 1.0);
}

// Рекурсивная отрисовка. Возвращает x-центр узла.
// found — узел, найденный Find (оранжевый), marked — узлы диапазона (зелёные).
// positions — центры узлов для центрирования вида на найденном.
template <class Node>
static double DrawNode(QGraphicsScene& scene, const Node* node, double left, int depth,
                       const QSet<quintptr>& found, const QSet<quintptr>& marked,
                       QHash<quintptr, QPointF>& positions) {
    const double units = SubtreeUnits(node);
    const double y = depth * kLevelH;

    // Сначала дети: раскладываем их слева направо, запоминая центры и номера веток.
    // left/pos — в пикселях, SubtreeUnits — в "единицах" раскладки.
    QVector<QPair<double, int>> childCenters;
    double pos = left;
    for (int i = 0; i < node->n; ++i) {
        const auto* child = node->children[i];
        const double w = SubtreeUnits(child) * kUnitW;
        if (child) {
            const double c = DrawNode(scene, child, pos, depth + 1, found, marked, positions);
            childCenters.append({c, i});
        }
        pos += w;
    }

    // Родитель — по центру над детьми (лист — по центру своей единицы).
    const double cx = childCenters.isEmpty()
        ? left + units * kUnitW / 2.0
        : (childCenters.first().first + childCenters.last().first) / 2.0;

    // Рёбра с номерами веток.
    QPen edgePen(QColor(120, 120, 120));
    for (const auto& [childCx, branch] : childCenters) {
        scene.addLine(cx, y + kNodeH, childCx, y + kLevelH, edgePen);
        auto* label = scene.addSimpleText(QString::number(branch));
        QFont f = label->font();
        f.setPointSize(7);
        label->setFont(f);
        label->setBrush(QColor(110, 110, 110));
        label->setPos((cx + childCx) / 2.0 + 3, (y + kNodeH + y + kLevelH) / 2.0 - 9);
    }

    // Узел: скруглённый прямоугольник + значение.
    const quintptr addr = reinterpret_cast<quintptr>(node);
    QColor fill(173, 216, 230);                       // обычный узел — голубой
    if (depth == 0) fill = QColor(100, 160, 220);     // корень — темнее
    if (marked.contains(addr)) fill = QColor(144, 238, 144);  // диапазон — зелёный
    if (found.contains(addr))  fill = QColor(255, 190, 90);   // найденный — оранжевый

    QPainterPath path;
    path.addRoundedRect(QRectF(cx - kNodeW / 2.0, y, kNodeW, kNodeH), 8, 8);
    scene.addPath(path, QPen(QColor(60, 60, 60)), QBrush(fill));

    auto* text = scene.addSimpleText(QString::number(node->data));
    text->setPos(cx - text->boundingRect().width() / 2.0,
                 y + kNodeH / 2.0 - text->boundingRect().height() / 2.0);

    positions.insert(addr, QPointF(cx, y + kNodeH / 2.0));
    return cx;
}

// Сбор адресов узлов со значениями в [min, max] (для подсветки диапазона).
template <class Node>
static void CollectInRange(const Node* node, int min, int max, QSet<quintptr>& out) {
    if (!node) return;
    if (node->data >= min && node->data <= max) {
        out.insert(reinterpret_cast<quintptr>(node));
    }
    for (int i = 0; i < node->n; ++i) CollectInRange(node->children[i], min, max, out);
}

// QGraphicsView с зумом колесом мыши и авто-вписыванием при ресайзе.
class TreeView : public QGraphicsView {
public:
    using QGraphicsView::QGraphicsView;
    bool autoFit = true;
protected:
    void wheelEvent(QWheelEvent* event) override {
        const double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        scale(factor, factor);
    }
    void resizeEvent(QResizeEvent* event) override {
        QGraphicsView::resizeEvent(event);
        // Вписываем после того, как viewport получил реальный размер
        // (первый показ окна, изменение размеров сплиттера/окна).
        if (autoFit && scene() && !scene()->items().isEmpty()) {
            fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        }
    }
};

// ---------------------------------------------------------------------------
// Главное окно
// ---------------------------------------------------------------------------

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(int n, int selectorKind, QWidget* parent = nullptr)
        : QMainWindow(parent), n_(n), selectorKind_(selectorKind) {
        tree_ = CreateTree(n_, selectorKind_);
        scene_ = new QGraphicsScene(this);
        BuildUi();
        RefreshScene();
        UpdateStatus();
        Log(QString("<b>Лабораторная работа №1.</b> n-арное дерево, вариант 4. "
                    "Графическое меню (аналог консольного main.cpp)."), "black");
    }

    ~MainWindow() override { delete tree_; }

    void SetEchoLog(bool on) { echoLog_ = on; }

    // Прогон всех операций без диалогов (запуск с --selftest, offscreen).
    int RunSelfTest() {
        int failures = 0;
        auto step = [&](const char* name, auto&& fn) {
            try { fn(); Log(QString("selftest: %1 — ok").arg(name), "gray"); }
            catch (const std::exception& e) {
                ++failures;
                Log(QString("selftest: %1 — ПРОВАЛ: %2").arg(name, e.what()), "red");
            }
        };

        step("вставка через селектор", [&] {
            for (int v : {50, 51, 52, 53, 54, 55}) { valueSpin_->setValue(v); DoInsert(); }
            if (tree_->GetSize() != 6) throw std::runtime_error("неверный размер");
        });
        step("массовый ввод", [&] {
            bulkEdit_->setText("10 20 30"); DoBulkInsert();
            if (tree_->GetSize() != 9) throw std::runtime_error("неверный размер");
        });
        step("случайные числа", [&] {
            countSpin_->setValue(5); maxSpin_->setValue(100); seedSpin_->setValue(42);
            DoRandomInsert();
            if (tree_->GetSize() != 14) throw std::runtime_error("неверный размер");
        });
        step("явная вставка в ветку корня", [&] {
            valueSpin_->setValue(7); branchSpin_->setValue(0); DoInsertIntoBranch();
            if (tree_->GetSize() != 15) throw std::runtime_error("неверный размер");
        });
        step("явная вставка по пути", [&] {
            valueSpin_->setValue(15); pathEdit_->setText("2 1"); DoInsertByPath();
            if (tree_->GetSize() != 16) throw std::runtime_error("неверный размер");
        });
        step("поиск", [&] { valueSpin_->setValue(51); DoFind(); });
        step("диапазон", [&] {
            minSpin_->setValue(10); maxRangeSpin_->setValue(50); DoRange();
            // Снимок окна для визуальной проверки отрисовки (16 узлов, подсветка).
            grab().save(QDir::tempPath() + "/lab1_gui_selftest.png");
        });
        step("обходы", [&] {
            for (int i = 0; i < 4; ++i) {
                orderCombo_->setCurrentIndex(i);
                if (i == 3) {
                    QString ord;
                    for (int k = 0; k <= n_; ++k) ord += QString::number(k) + ' ';
                    orderEdit_->setText(ord.trimmed());
                }
                DoSequence();
            }
        });
        step("DFS-итератор", [&] { DoIterator(); });
        step("тест интерфейсов", [&] { valueSpin_->setValue(50); DoTestInterfaces(); });
        step("удаление", [&] {
            valueSpin_->setValue(51); DoRemove();
            if (tree_->GetSize() != 15) throw std::runtime_error("неверный размер");
        });
        step("удаление несуществующего (ожидаемая ошибка)", [&] {
            bool thrown = false;
            try { tree_->Remove(999999); } catch (const std::exception&) { thrown = true; }
            if (!thrown) throw std::runtime_error("ожидалось исключение");
        });
        step("очистка", [&] {
            tree_->Clear(); ClearHighlights(); RefreshScene(); UpdateStatus();
            if (!tree_->IsEmpty()) throw std::runtime_error("дерево не пусто");
        });
        step("пересоздание (CompareSelector)", [&] {
            RecreateTree(3, 1);
            for (int v : {10, 5, 15}) { valueSpin_->setValue(v); DoInsert(); }
            if (tree_->GetSize() != 3) throw std::runtime_error("неверный размер");
        });
        step("режим без селектора", [&] {
            RecreateTree(2, 0);
            valueSpin_->setValue(1); branchSpin_->setValue(0); DoInsertIntoBranch();
            valueSpin_->setValue(2); pathEdit_->setText("1"); DoInsertByPath();
            bool thrown = false;
            try { tree_->Insert(3); } catch (const std::exception&) { thrown = true; }
            if (!thrown) throw std::runtime_error("Insert без селектора должен бросать");
            if (tree_->GetSize() != 2) throw std::runtime_error("неверный размер");
        });

        Log(failures == 0 ? "<b>SELFTEST OK</b>"
                          : QString("<b>SELFTEST FAILED: %1</b>").arg(failures),
            failures == 0 ? "green" : "red");
        return failures == 0 ? 0 : 1;
    }

private:
    NTree<int>* tree_;
    int n_;
    int selectorKind_;

    QGraphicsScene* scene_;
    TreeView* view_;
    QTextEdit* log_;
    QLabel* statusLabel_;
    QCheckBox* autoFitCheck_;

    QSpinBox* valueSpin_;
    QSpinBox* branchSpin_;
    QLineEdit* pathEdit_;
    QLineEdit* bulkEdit_;
    QSpinBox* countSpin_;
    QSpinBox* maxSpin_;
    QSpinBox* seedSpin_;
    QComboBox* orderCombo_;
    QLineEdit* orderEdit_;
    QSpinBox* minSpin_;
    QSpinBox* maxRangeSpin_;

    QSet<quintptr> found_;     // подсветка результата Find
    QSet<quintptr> marked_;    // подсветка диапазона
    QHash<quintptr, QPointF> positions_;
    bool echoLog_ = false;     // режим --selftest: дублировать журнал в stdout

    // ------------------------- построение интерфейса -------------------------

    void BuildUi() {
        setWindowTitle(QString("Лабораторная №1 — n-арное дерево (n=%1)").arg(n_));
        resize(1200, 760);

        auto* splitter = new QSplitter(Qt::Horizontal, this);
        setCentralWidget(splitter);

        // Левая панель операций (прокручиваемая).
        auto* panel = new QWidget();
        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(6, 6, 6, 6);

        panelLayout->addWidget(MakeTreeGroup());
        panelLayout->addWidget(MakeValueGroup());
        panelLayout->addWidget(MakeExplicitGroup());
        panelLayout->addWidget(MakeBulkGroup());
        panelLayout->addWidget(MakeTraversalGroup());
        panelLayout->addStretch();

        auto* scroll = new QScrollArea();
        scroll->setWidget(panel);
        scroll->setWidgetResizable(true);
        scroll->setMinimumWidth(360);
        scroll->setFrameShape(QFrame::NoFrame);
        splitter->addWidget(scroll);

        // Правая часть: вид дерева + журнал.
        auto* right = new QSplitter(Qt::Vertical);

        auto* viewHost = new QWidget();
        auto* viewLayout = new QVBoxLayout(viewHost);
        viewLayout->setContentsMargins(0, 0, 0, 0);

        auto* viewBar = new QHBoxLayout();
        auto* fitBtn = new QPushButton("Вписать");
        auto* zoomInBtn = new QPushButton("+");
        auto* zoomOutBtn = new QPushButton("−");
        autoFitCheck_ = new QCheckBox("Вписывать при изменениях");
        autoFitCheck_->setChecked(true);
        viewBar->addWidget(fitBtn);
        viewBar->addWidget(zoomInBtn);
        viewBar->addWidget(zoomOutBtn);
        viewBar->addWidget(autoFitCheck_);
        viewBar->addStretch();
        viewLayout->addLayout(viewBar);

        view_ = new TreeView(scene_);
        view_->setRenderHint(QPainter::Antialiasing);
        view_->setDragMode(QGraphicsView::ScrollHandDrag);
        viewLayout->addWidget(view_);
        right->addWidget(viewHost);

        log_ = new QTextEdit();
        log_->setReadOnly(true);
        log_->setPlaceholderText("Журнал операций");
        right->addWidget(log_);
        right->setStretchFactor(0, 3);
        right->setStretchFactor(1, 1);
        splitter->addWidget(right);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);

        connect(fitBtn, &QPushButton::clicked, this, [this] { FitView(); });
        connect(zoomInBtn, &QPushButton::clicked, this, [this] { view_->scale(1.2, 1.2); });
        connect(zoomOutBtn, &QPushButton::clicked, this, [this] { view_->scale(1.0 / 1.2, 1.0 / 1.2); });
        connect(autoFitCheck_, &QCheckBox::toggled, this, [this](bool on) {
            view_->autoFit = on;
            if (on) FitView();
        });

        statusLabel_ = new QLabel(this);
        statusBar()->addPermanentWidget(statusLabel_);
    }

    QGroupBox* MakeTreeGroup() {
        auto* group = new QGroupBox("Дерево");
        auto* layout = new QVBoxLayout(group);
        auto* row1 = new QHBoxLayout();
        auto* recreateBtn = new QPushButton("Пересоздать… (п.12)");
        auto* clearBtn = new QPushButton("Очистить (п.5)");
        row1->addWidget(recreateBtn);
        row1->addWidget(clearBtn);
        layout->addLayout(row1);
        auto* sizeBtn = new QPushButton("Размер / пусто (п.4)");
        layout->addWidget(sizeBtn);

        connect(recreateBtn, &QPushButton::clicked, this, &MainWindow::DoRecreate);
        connect(clearBtn, &QPushButton::clicked, this, &MainWindow::DoClear);
        connect(sizeBtn, &QPushButton::clicked, this, &MainWindow::DoSize);
        return group;
    }

    QGroupBox* MakeValueGroup() {
        auto* group = new QGroupBox("Значение (пункты 1–3, 13–15)");
        auto* layout = new QVBoxLayout(group);
        valueSpin_ = new QSpinBox();
        valueSpin_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        valueSpin_->setValue(0);
        layout->addWidget(valueSpin_);
        auto* row = new QHBoxLayout();
        auto* insertBtn = new QPushButton("Вставить (п.1)");
        auto* removeBtn = new QPushButton("Удалить (п.2)");
        auto* findBtn = new QPushButton("Найти (п.3)");
        row->addWidget(insertBtn);
        row->addWidget(removeBtn);
        row->addWidget(findBtn);
        layout->addLayout(row);

        connect(insertBtn, &QPushButton::clicked, this, &MainWindow::DoInsert);
        connect(removeBtn, &QPushButton::clicked, this, &MainWindow::DoRemove);
        connect(findBtn, &QPushButton::clicked, this, &MainWindow::DoFind);
        return group;
    }

    QGroupBox* MakeExplicitGroup() {
        auto* group = new QGroupBox("Явная вставка без селектора (пункты 14–15)");
        auto* layout = new QFormLayout(group);
        branchSpin_ = new QSpinBox();
        branchSpin_->setRange(0, n_ - 1);
        auto* branchBtn = new QPushButton("Вставить в ветку корня (п.14)");
        layout->addRow("Ветка корня:", branchSpin_);
        layout->addRow(branchBtn);
        pathEdit_ = new QLineEdit();
        pathEdit_->setPlaceholderText("например: 2 1 (пусто — в корень)");
        auto* pathBtn = new QPushButton("Вставить по пути (п.15)");
        layout->addRow("Путь из индексов:", pathEdit_);
        layout->addRow(pathBtn);

        connect(branchBtn, &QPushButton::clicked, this, &MainWindow::DoInsertIntoBranch);
        connect(pathBtn, &QPushButton::clicked, this, &MainWindow::DoInsertByPath);
        return group;
    }

    QGroupBox* MakeBulkGroup() {
        auto* group = new QGroupBox("Массовый ввод (пункты 10–11)");
        auto* layout = new QVBoxLayout(group);
        bulkEdit_ = new QLineEdit();
        bulkEdit_->setPlaceholderText("числа через пробел, например: 50 51 52");
        auto* bulkBtn = new QPushButton("Вставить числа (п.10)");
        layout->addWidget(bulkEdit_);
        layout->addWidget(bulkBtn);

        auto* grid = new QGridLayout();
        countSpin_ = new QSpinBox(); countSpin_->setRange(1, 100000); countSpin_->setValue(10);
        maxSpin_ = new QSpinBox(); maxSpin_->setRange(0, 1000000000); maxSpin_->setValue(100);
        seedSpin_ = new QSpinBox(); seedSpin_->setRange(0, std::numeric_limits<int>::max()); seedSpin_->setValue(0);
        grid->addWidget(new QLabel("Количество:"), 0, 0); grid->addWidget(countSpin_, 0, 1);
        grid->addWidget(new QLabel("Макс. значение:"), 1, 0); grid->addWidget(maxSpin_, 1, 1);
        grid->addWidget(new QLabel("Seed (0 = случайный):"), 2, 0); grid->addWidget(seedSpin_, 2, 1);
        layout->addLayout(grid);
        auto* randomBtn = new QPushButton("Случайные числа (п.11)");
        layout->addWidget(randomBtn);

        connect(bulkBtn, &QPushButton::clicked, this, &MainWindow::DoBulkInsert);
        connect(randomBtn, &QPushButton::clicked, this, &MainWindow::DoRandomInsert);
        return group;
    }

    QGroupBox* MakeTraversalGroup() {
        auto* group = new QGroupBox("Обходы и анализ (пункты 6–9, 13)");
        auto* layout = new QVBoxLayout(group);

        orderCombo_ = new QComboBox();
        orderCombo_->addItem("К123 — прямой (п.6)");
        orderCombo_->addItem("123К — обратный");
        orderCombo_->addItem("1К23 — симметричный");
        orderCombo_->addItem("Свой порядок");
        layout->addWidget(orderCombo_);
        orderEdit_ = new QLineEdit();
        orderEdit_->setPlaceholderText(QString("n+1 чисел: 0 — узел, 1..n — ветки (для n=%1)").arg(n_));
        orderEdit_->setEnabled(false);
        layout->addWidget(orderEdit_);
        auto* row1 = new QHBoxLayout();
        auto* seqBtn = new QPushButton("Обход (п.7)");
        auto* iterBtn = new QPushButton("DFS-итератор (п.9)");
        row1->addWidget(seqBtn);
        row1->addWidget(iterBtn);
        layout->addLayout(row1);

        auto* row2 = new QHBoxLayout();
        minSpin_ = new QSpinBox();
        minSpin_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        maxRangeSpin_ = new QSpinBox();
        maxRangeSpin_->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        maxRangeSpin_->setValue(100);
        auto* rangeBtn = new QPushButton("Диапазон [min, max] (п.8)");
        row2->addWidget(new QLabel("min:"));
        row2->addWidget(minSpin_);
        row2->addWidget(new QLabel("max:"));
        row2->addWidget(maxRangeSpin_);
        row2->addWidget(rangeBtn);
        layout->addLayout(row2);

        auto* testBtn = new QPushButton("Тест ICollection/IEnumerable и операторов (п.13)");
        layout->addWidget(testBtn);

        connect(orderCombo_, &QComboBox::currentIndexChanged, this, [this](int idx) {
            orderEdit_->setEnabled(idx == 3);
        });
        connect(seqBtn, &QPushButton::clicked, this, &MainWindow::DoSequence);
        connect(iterBtn, &QPushButton::clicked, this, &MainWindow::DoIterator);
        connect(rangeBtn, &QPushButton::clicked, this, &MainWindow::DoRange);
        connect(testBtn, &QPushButton::clicked, this, &MainWindow::DoTestInterfaces);
        return group;
    }

    // ------------------------- журнал и статус -------------------------

    void Log(const QString& html, const char* color = "black") {
        log_->append(QString("<span style=\"color:%1\">%2</span>").arg(color, html));
        if (echoLog_) {
            QTextDocument doc;
            doc.setHtml(html);
            fprintf(stdout, "%s\n", qPrintable(doc.toPlainText()));
            fflush(stdout);
        }
    }

    void LogSequence(const QString& title, const Sequence<int>& seq) {
        QString s;
        if (seq.GetLength() == 0) {
            s = "(пусто)";
        } else {
            for (size_t i = 0; i < seq.GetLength(); ++i) {
                s += QString::number(seq.Get(i));
                if (i + 1 < seq.GetLength()) s += ' ';
            }
        }
        Log(title + ": " + s.toHtmlEscaped(), "navy");
    }

    void UpdateStatus() {
        statusLabel_->setText(QString("n=%1 · элементов=%2 · стратегия: %3")
                                  .arg(n_).arg(tree_->GetSize()).arg(SelectorName(selectorKind_)));
        setWindowTitle(QString("Лабораторная №1 — n-арное дерево (n=%1)").arg(n_));
    }

    void ClearHighlights() { found_.clear(); marked_.clear(); }

    void RefreshScene() {
        scene_->clear();
        positions_.clear();
        if (auto* root = tree_->GetRoot()) {
            DrawNode(*scene_, root, 0.0, 0, found_, marked_, positions_);
        }
        scene_->setSceneRect(scene_->itemsBoundingRect().adjusted(-24, -24, 24, 24));
        if (autoFitCheck_->isChecked()) FitView();
    }

    void FitView() {
        if (!scene_->items().isEmpty()) {
            view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
        }
    }

    void AfterMutation() {
        ClearHighlights();
        RefreshScene();
        UpdateStatus();
    }

    // ------------------------- операции (аналоги Do* из main.cpp) -------------------------

    void DoInsert() {                                   // пункт 1
        const int value = valueSpin_->value();
        try {
            tree_->Insert(value);
            Log(QString("Вставлено %1.").arg(value));
            AfterMutation();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void DoRemove() {                                   // пункт 2
        if (tree_->IsEmpty()) { Log("Дерево пусто."); return; }
        const int value = valueSpin_->value();
        try {
            tree_->Remove(value);
            Log(QString("Удалено %1.").arg(value));
            AfterMutation();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void DoFind() {                                     // пункт 3
        if (tree_->IsEmpty()) { Log("Дерево пусто."); return; }
        const int value = valueSpin_->value();
        const auto* node = tree_->Find(value);
        ClearHighlights();
        if (node) {
            found_.insert(reinterpret_cast<quintptr>(node));
            Log(QString("Найдено: %1 (узел подсвечен).").arg(node->data), "green");
        } else {
            Log(QString("Не найдено: %1.").arg(value));
        }
        RefreshScene();
        if (node) {
            const auto it = positions_.find(reinterpret_cast<quintptr>(node));
            if (it != positions_.end()) view_->centerOn(it.value());
        }
    }

    void DoSize() {                                     // пункт 4
        Log(QString("Размер: %1").arg(tree_->GetSize()));
        Log(QString("Пусто: %1").arg(tree_->IsEmpty() ? "да" : "нет"));
    }

    void DoClear() {                                    // пункт 5
        if (tree_->IsEmpty()) { Log("Дерево уже пусто."); return; }
        const auto answer = QMessageBox::question(
            this, "Очистка", "Вы уверены, что хотите очистить дерево?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            tree_->Clear();
            Log("Дерево очищено.");
            AfterMutation();
        } else {
            Log("Отмена.");
        }
    }

    void DoSequence() {                                 // пункты 6–7
        if (tree_->IsEmpty()) { Log("Дерево пусто."); return; }
        const int type = orderCombo_->currentIndex();
        MutableArraySequence<int> order;
        if (type == 0) {                                // К123
            order.Append(0);
            for (int i = 1; i <= n_; ++i) order.Append(i);
        } else if (type == 1) {                         // 123К
            for (int i = 1; i <= n_; ++i) order.Append(i);
            order.Append(0);
        } else if (type == 2) {                         // 1К23
            order.Append(1);
            order.Append(0);
            for (int i = 2; i <= n_; ++i) order.Append(i);
        } else {                                        // свой порядок
            std::istringstream ss(orderEdit_->text().toStdString());
            int idx;
            while (ss >> idx) order.Append(idx);
            if (order.GetLength() != static_cast<size_t>(n_ + 1)) {
                Log(QString("Ошибка: порядок должен содержать ровно %1 чисел (0 и 1..%2).")
                        .arg(n_ + 1).arg(n_), "red");
                return;
            }
        }
        try {
            Sequence<int>* result = tree_->GetSequence(&order);
            LogSequence(type == 0 ? "Обход К123" : "Результат обхода", *result);
            delete result;
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void DoRange() {                                    // пункт 8
        if (tree_->IsEmpty()) { Log("Дерево пусто."); return; }
        const int lo = minSpin_->value();
        const int hi = maxRangeSpin_->value();
        try {
            Sequence<int>* result = tree_->GetRange(lo, hi);
            LogSequence(QString("Элементы в [%1, %2]").arg(lo).arg(hi), *result);
            delete result;
            ClearHighlights();
            if (lo <= hi) {
                if (const auto* root = tree_->GetRoot()) CollectInRange(root, lo, hi, marked_);
            }
            RefreshScene();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void DoIterator() {                                 // пункт 9
        if (tree_->IsEmpty()) { Log("Дерево пусто."); return; }
        QString s;
        int count = 0;
        for (auto it = tree_->begin(); it != tree_->end(); ++it) {
            s += QString::number(*it) + ' ';
            ++count;
        }
        Log("DFS-обход (старый итератор): " + s.toHtmlEscaped(), "navy");
        Log(QString("(посещено узлов: %1)").arg(count));
    }

    void DoBulkInsert() {                               // пункт 10
        std::istringstream ss(bulkEdit_->text().toStdString());
        int value;
        int ok = 0, fail = 0;
        while (ss >> value) {
            try { tree_->Insert(value); ++ok; }
            catch (...) { ++fail; }
        }
        Log(QString("Вставлено: %1, ошибок: %2").arg(ok).arg(fail));
        if (ok > 0) AfterMutation();
    }

    void DoRandomInsert() {                             // пункт 11
        const int count = countSpin_->value();
        const int maxVal = maxSpin_->value();
        int seed = seedSpin_->value();
        if (seed == 0) {
            std::random_device rd;
            seed = static_cast<int>(rd() & 0x7FFFFFFF);
            if (seed == 0) seed = 1;
        }
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> dist(0, maxVal);
        try {
            for (int i = 0; i < count; ++i) tree_->Insert(dist(rng));
            Log(QString("Вставлено %1 случайных элементов (seed=%2).").arg(count).arg(seed));
            AfterMutation();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void RecreateTree(int n, int kind) {
        delete tree_;
        n_ = n;
        selectorKind_ = kind;
        tree_ = CreateTree(n_, selectorKind_);
        branchSpin_->setRange(0, n_ - 1);
        orderEdit_->setPlaceholderText(QString("n+1 чисел: 0 — узел, 1..n — ветки (для n=%1)").arg(n_));
        AfterMutation();
    }

    void DoRecreate() {                                 // пункт 12
        int n = n_, kind = selectorKind_;
        if (!AskTreeParams(this, n, kind)) { Log("Отмена."); return; }
        RecreateTree(n, kind);
        Log(QString("Дерево пересоздано (n=%1, стратегия: %2).").arg(n).arg(SelectorName(kind)));
    }

    void DoTestInterfaces() {                           // пункт 13
        Log("<b>--- Тест ICollection / IEnumerable и операторов ---</b>");
        Log(QString("1. ICollection: GetSize() = %1, IsEmpty() = %2")
                .arg(tree_->GetSize()).arg(tree_->IsEmpty() ? "true" : "false"));
        const int val = valueSpin_->value();
        Log(QString("   Contains(%1) = %2").arg(val).arg(tree_->Contains(val) ? "true" : "false"));

        QString s;
        IEnumerator<int>* enumerator = tree_->GetEnumerator();
        while (enumerator->MoveNext()) s += QString::number(enumerator->GetCurrent()) + ' ';
        delete enumerator;
        Log("2. IEnumerable (IEnumerator): " + (s.isEmpty() ? QString("(пусто)") : s.toHtmlEscaped()), "navy");

        std::ostringstream os;
        os << *tree_;
        Log(QString("3. operator&lt;&lt;: %1").arg(QString::fromStdString(os.str()).toHtmlEscaped()));

        NTree<int> tree2(3);
        tree2.SetChildSelector(CompareSelector<int>());
        tree2.Insert(10);
        tree2.Insert(5);
        Log("   Создали второе дерево {10, 5} (CompareSelector, n=3).");
        Log(QString("   (*this == tree2) = %1").arg(*tree_ == tree2 ? "true" : "false"));
        Log(QString("   (*this != tree2) = %1").arg(*tree_ != tree2 ? "true" : "false"));
    }

    void DoInsertIntoBranch() {                         // пункт 14
        const int value = valueSpin_->value();
        const int index = branchSpin_->value();
        try {
            tree_->Insert(value, index);
            Log(QString("Вставлено %1 в ветку %2.").arg(value).arg(index));
            AfterMutation();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }

    void DoInsertByPath() {                             // пункт 15
        const int value = valueSpin_->value();
        std::istringstream ss(pathEdit_->text().toStdString());
        MutableArraySequence<int> path;
        int idx;
        while (ss >> idx) path.Append(idx);
        try {
            tree_->Insert(value, &path);
            Log(QString("Вставлено %1 по пути длины %2.").arg(value).arg(path.GetLength()));
            AfterMutation();
        } catch (const std::exception& e) {
            Log(QString("Ошибка: %1").arg(e.what()), "red");
        }
    }
};


int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Lab1 — n-арное дерево");

    const bool selftest = app.arguments().contains("--selftest");

    int n = 3;
    int kind = 2;
    if (!selftest && !AskTreeParams(nullptr, n, kind)) {
        return 0;
    }
    if (selftest) { n = 4; kind = 2; }

    MainWindow window(n, kind);
    window.show();

    if (selftest) {
        window.SetEchoLog(true);
        QTimer::singleShot(0, &window, [&window] {
            QApplication::exit(window.RunSelfTest());
        });
    }
    return app.exec();
}

#include "main_qt.moc"