#include "mainwindow.h"
#include "codegenerate.h"
#include "qlabel.h"
#include "qscrollarea.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPlainTextEdit>
#include "regex.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 实时更新输入文本框中的内容到inputContent中
    connect(ui->input_area, &QPlainTextEdit::textChanged, this, [=]() {
        inputContent = ui->input_area->toPlainText();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_do_convert_clicked()
{
    if (inputContent.size() != 0) {
        regexs.clear();
        QStringList regexContent = inputContent.split("\n");
        for (QString regex : regexContent) {
            Regex re = Regex(regex.toStdString());
            preprocessRegex(re);
            initNFA(re);
            initDFA(re);
            initMinDFA(re);
            regexs.push_back(re);
        }
    }
}


void MainWindow::on_pushButton_import_regex_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(nullptr, "选择文件", "", "txt (*.txt);; 其他格式文本 (*.*)");
    if (filePath.isEmpty()) return;
    QString in = getFileContent(filePath);
    if (!in.isEmpty()) {
        inputContent = in;
        ui->input_area->setPlainText(inputContent);
    }
}

void displayNFATransitions(QList<Regex>& regexs, QScrollArea* parent) {
    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(parent); // 布局管理器
    container->setMinimumWidth(parent->width() - 20);
    for (auto& regex : regexs) {
        std::set<std::string> symbolSet;
        std::set<int> stateSet;

        const auto& transitions = regex.nfa.transitions;
        for (const auto& transition : transitions) {
            symbolSet.insert(transition.symbol);
            stateSet.insert(transition.from);
            stateSet.insert(transition.to);
        }
        QStringList headers;
        headers << "状态标记" << "状态编号";  // 状态标识列和状态编号列
        for (const auto& symbol : symbolSet) {
            headers << QString::fromStdString(symbol);
        }

        QLabel* label = new QLabel(QString::fromStdString(regex.rawRegex));
        layout->addWidget(label);
        // 为每个正则表达式创建一个新的表格
        QTableWidget* tableWidget = new QTableWidget(parent);
        tableWidget->setColumnCount(2 + symbolSet.size());
        tableWidget->setHorizontalHeaderLabels(headers);

        // 设置行数
        tableWidget->setRowCount(stateSet.size());

        // 填充表格数据
        for (const auto& transition : transitions) {
            QTableWidgetItem* itemFrom = new QTableWidgetItem(QString::number(transition.from));
            itemFrom->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(transition.from, 1, itemFrom);

            int col = headers.indexOf(QString::fromStdString(transition.symbol));
            QTableWidgetItem* itemTo = tableWidget->item(transition.from, col);
            if (itemTo == nullptr) {
                itemTo = new QTableWidgetItem(QString::number(transition.to));
            } else {
                itemTo->setText(itemTo->text() + ", " + QString::number(transition.to));
            }
            itemTo->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(transition.from, col, itemTo);
        }

        QTableWidgetItem* startIconItem = new QTableWidgetItem("-");
        startIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
        startIconItem->setForeground(Qt::red);  // 设置文本颜色为红色
        tableWidget->setItem(regex.nfa.startState, 0, startIconItem);

        for (int accept : regex.nfa.acceptStates) {
            QTableWidgetItem* acceptIconItem = new QTableWidgetItem("+");
            acceptIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            acceptIconItem->setForeground(Qt::red);  // 设置文本颜色为红色
            tableWidget->setItem(accept, 0, acceptIconItem);

            QTableWidgetItem* acceptItem = new QTableWidgetItem(QString::number(accept));
            acceptItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(accept, 1, acceptItem);
        }
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget->setMinimumHeight((stateSet.size() + 1) * 30 + 20);
        tableWidget->setMaximumHeight((stateSet.size() + 1) * 30 + 20);
        // 添加表格到布局
        layout->addWidget(tableWidget);
    }
    container->setLayout(layout);
    parent->setWidgetResizable(false);
    parent->setWidget(container);
}


void displayDFATransitions(QList<Regex>& regexs, QScrollArea* parent) {
    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(parent); // 布局管理器
    container->setMinimumWidth(parent->width() - 20);
    for (auto& regex : regexs) {
        if (!regex.id.empty() && regex.id[0] != '_') continue; // 符合条件的才生成DFA

        std::set<std::string> symbolSet;
        std::set<std::string> stateSet;

        const auto& transitions = regex.dfa.transitions;
        for (DFATransition transition : transitions) {
            symbolSet.insert(transition.symbol);
            stateSet.insert(stateToString(transition.from));
            stateSet.insert(stateToString(transition.to));
        }
        QStringList headers;
        headers << "状态标记" << "状态集";  // 状态标识列和状态编号列
        for (const auto& symbol : symbolSet) {
            headers << QString::fromStdString(symbol);
        }

        QLabel* label = new QLabel(QString::fromStdString(regex.rawRegex));
        layout->addWidget(label);
        // 为每个正则表达式创建一个新的表格
        QTableWidget* tableWidget = new QTableWidget(parent);
        tableWidget->setColumnCount(2 + symbolSet.size());
        tableWidget->setHorizontalHeaderLabels(headers);

        // 设置行数
        tableWidget->setRowCount(stateSet.size());

        // 填充表格数据
        for (DFATransition transition : transitions) {
            // 计算元素的位置（索引）
            std::string fromContent = stateToString(transition.from);
            auto it = stateSet.find(fromContent);
            int fromIndex = 0;
            for (auto iter = stateSet.begin(); iter != it; ++iter) {
                ++fromIndex;
            }

            QTableWidgetItem* itemFrom = new QTableWidgetItem(QString::fromStdString(stateToString(transition.from)));
            itemFrom->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(fromIndex, 1, itemFrom);

            int col = headers.indexOf(QString::fromStdString(transition.symbol));
            QTableWidgetItem* itemTo = new QTableWidgetItem(QString::fromStdString(stateToString(transition.to)));
            itemTo->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(fromIndex, col, itemTo);
        }

        QTableWidgetItem* startIconItem = new QTableWidgetItem("-");
        startIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
        startIconItem->setForeground(Qt::red);  // 设置文本颜色为红色
        tableWidget->setItem(*regex.dfa.startState.rbegin(), 0, startIconItem);


        for (auto &accept : regex.dfa.acceptStates) {
            QTableWidgetItem* acceptIconItem = new QTableWidgetItem("+");
            acceptIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            acceptIconItem->setForeground(Qt::red);  // 设置文本颜色为红色

            std::string acceptContent = stateToString(accept);

            // 计算元素的位置（索引）
            auto it = stateSet.find(acceptContent);
            int acceptIndex = 0;
            for (auto iter = stateSet.begin(); iter != it; ++iter) {
                ++acceptIndex;
            }

            tableWidget->setItem(acceptIndex, 0, acceptIconItem);

            QTableWidgetItem* acceptItem = new QTableWidgetItem(QString::fromStdString(acceptContent));
            acceptItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(acceptIndex, 1, acceptItem);
        }


        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget->setMinimumHeight((stateSet.size() + 1) * 30 + 20);
        tableWidget->setMaximumHeight((stateSet.size() + 1) * 30 + 20);
        // 添加表格到布局
        layout->addWidget(tableWidget);
    }
    container->setLayout(layout);
    parent->setWidgetResizable(false);
    parent->setWidget(container);
}

void displayMinDFATransitions(QList<Regex>& regexs, QScrollArea* parent) {
    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(parent); // 布局管理器
    container->setMinimumWidth(parent->width() - 20);
    for (auto& regex : regexs) {
        if (!regex.id.empty() && regex.id[0] != '_') continue; // 符合条件的才生成DFA

        std::set<std::string> symbolSet;
        std::set<int> stateSet;

        const auto& transitions = regex.minDfa.transitions;
        for (const auto& transition : transitions) {
            symbolSet.insert(transition.symbol);
            stateSet.insert(*transition.from.rbegin());
            stateSet.insert(*transition.to.rbegin());
        }
        QStringList headers;
        headers << "状态标记" << "状态编号";  // 状态标识列和状态编号列
        for (const auto& symbol : symbolSet) {
            headers << QString::fromStdString(symbol);
        }

        QLabel* label = new QLabel(QString::fromStdString(regex.rawRegex));
        layout->addWidget(label);
        // 为每个正则表达式创建一个新的表格
        QTableWidget* tableWidget = new QTableWidget(parent);
        tableWidget->setColumnCount(2 + symbolSet.size());
        tableWidget->setHorizontalHeaderLabels(headers);

        // 设置行数
        tableWidget->setRowCount(stateSet.size());

        // 填充表格数据
        for (const auto& transition : transitions) {
            QTableWidgetItem* itemFrom = new QTableWidgetItem(QString::number(*transition.from.rbegin()));
            itemFrom->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(*transition.from.rbegin(), 1, itemFrom);

            int col = headers.indexOf(QString::fromStdString(transition.symbol));
            QTableWidgetItem* itemTo = tableWidget->item(*transition.from.rbegin(), col);
            if (itemTo == nullptr) {
                itemTo = new QTableWidgetItem(QString::number(*transition.to.rbegin()));
            } else {
                itemTo->setText(itemTo->text() + ", " + QString::number(*transition.to.rbegin()));
            }
            itemTo->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(*transition.from.rbegin(), col, itemTo);
        }

        QTableWidgetItem* startIconItem = new QTableWidgetItem("-");
        startIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
        startIconItem->setForeground(Qt::red);  // 设置文本颜色为红色
        tableWidget->setItem(*regex.minDfa.startState.rbegin(), 0, startIconItem);

        for (auto &accept : regex.minDfa.acceptStates) {
            QTableWidgetItem* acceptIconItem = new QTableWidgetItem("+");
            acceptIconItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            acceptIconItem->setForeground(Qt::red);  // 设置文本颜色为红色
            tableWidget->setItem(*accept.rbegin(), 0, acceptIconItem);

            QTableWidgetItem* acceptItem = new QTableWidgetItem(QString::number(*accept.rbegin()));
            acceptItem->setTextAlignment(Qt::AlignCenter);  // 设置居中
            tableWidget->setItem(*accept.rbegin(), 1, acceptItem);
        }
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget->setMinimumHeight((stateSet.size() + 1) * 30 + 20);
        tableWidget->setMaximumHeight((stateSet.size() + 1) * 30 + 20);
        // 添加表格到布局
        layout->addWidget(tableWidget);
    }
    container->setLayout(layout);
    parent->setWidgetResizable(false);
    parent->setWidget(container);
}

void MainWindow::on_pushButton_show_nfa_clicked()
{
    displayNFATransitions(regexs, ui->output_area);
}


void MainWindow::on_pushButton_show_dfa_clicked()
{
    displayDFATransitions(regexs, ui->output_area);
}


void MainWindow::on_pushButton_show_mdfa_clicked()
{
    displayMinDFATransitions(regexs, ui->output_area);
}


void MainWindow::on_pushButton_show_code_clicked()
{
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(ui->output_area);
    container->setMinimumWidth(ui->output_area->width() - 20);

    for (auto &regex : regexs)
    {
        QLabel *label = new QLabel(QString::fromStdString(regex.rawRegex));
        layout->addWidget(label);
        QPlainTextEdit *text = new QPlainTextEdit;
        text->setReadOnly(true);
        text->setPlainText(generateLexerCodeForRegex(regex, regexs));
        layout->addWidget(text);
    }

    container->setLayout(layout);
    ui->output_area->setWidgetResizable(false);
    ui->output_area->setWidget(container);
}

QString MainWindow::getFileContent(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    QString fileContent = in.readAll();
    file.close();
    return fileContent;
}

