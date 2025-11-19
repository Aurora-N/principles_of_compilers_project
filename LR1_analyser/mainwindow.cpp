#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_save_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("保存文件"), "文法.txt", tr("文本文件 (*.txt);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << ui->inputEditor->toPlainText();
    file.close();
}

void MainWindow::on_pushButton_open_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("打开文件"), "", tr("文本文件 (*.txt);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    ui->inputEditor->setPlainText(in.readAll());
    file.close();
}

void MainWindow::clearOutputWidgets() {
    ui->firstSetListWidget->clear();
    ui->followSetListWidget->clear();
    
    ui->LR0DFATableWidget->clear();
    ui->LR0DFATableWidget->setRowCount(0);
    ui->LR0DFATableWidget->setColumnCount(0);
    
    ui->SLRStatusOutput->clear();
    
    ui->LR1DFATableWidget->clear();
    ui->LR1DFATableWidget->setRowCount(0);
    ui->LR1DFATableWidget->setColumnCount(0);
    
    ui->LR1AnalyzeTableWidget->clear();
    ui->LR1AnalyzeTableWidget->setRowCount(0);
    ui->LR1AnalyzeTableWidget->setColumnCount(0);
}

void MainWindow::displayFirstFollowSets() {
    const auto& firstSets = grammar.getFirstSets();
    for (const auto& pair : firstSets) {
        QString itemText = QString::fromStdString(pair.first) + ": { ";
        QStringList elements;
        for (const string& symbol : pair.second) {
            elements.append(QString::fromStdString(symbol));
        }
        itemText += elements.join(", ") + " }";
        ui->firstSetListWidget->addItem(itemText);
    }

    const auto& followSets = grammar.getFollowSets();
    for (const auto& pair : followSets) {
        QString itemText = QString::fromStdString(pair.first) + ": { ";
        QStringList elements;
        for (const string& symbol : pair.second) {
            elements.append(QString::fromStdString(symbol));
        }
        itemText += elements.join(", ") + " }";
        ui->followSetListWidget->addItem(itemText);
    }
}

void MainWindow::displayLR0DFA() {
    const auto& lr0States = grammar.getLR0States();
    const auto& lr0Transitions = grammar.getLR0Transitions();
    const auto& productions = grammar.getProductions();

    if (lr0States.empty() && grammar.getErrorMessage().empty()) { 
      return;
    } else if (lr0States.empty()) {
       return;
    }
    
    QStringList headerLabels;
    headerLabels << tr("状态ID") << tr("LR(0)项");
    
    set<string> allSymbolsSet;
    
    for (const auto& nt : grammar.getNonTerminals()) {
         if (nt != Grammar::AUGMENTED_START_SYMBOL || nt == grammar.getStartSymbol()) {
            allSymbolsSet.insert(nt);
         }
    }
    for (const auto& t : grammar.getTerminals()) {
        if (t != Grammar::EPSILON) {
            allSymbolsSet.insert(t);
        }
    }

    QStringList symbolHeaders;
    for(const string& sym : allSymbolsSet) {
        symbolHeaders << QString::fromStdString(sym);
    }
    headerLabels.append(symbolHeaders);
    ui->LR0DFATableWidget->setColumnCount(headerLabels.size());
    ui->LR0DFATableWidget->setHorizontalHeaderLabels(headerLabels);

    ui->LR0DFATableWidget->setRowCount(lr0States.size());
    for (size_t i = 0; i < lr0States.size(); ++i) {
        ui->LR0DFATableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        QString itemsString;
        for (const auto& item : lr0States[i]) {
            itemsString += QString::fromStdString(item.toString(productions)) + "\n";
        }
        QTableWidgetItem* itemsCell = new QTableWidgetItem(itemsString.trimmed());
        ui->LR0DFATableWidget->setItem(i, 1, itemsCell);

        int symbolColIndex = 2;
        for (const string& symStr : allSymbolsSet) {
            if (lr0Transitions.count(i)) {
                const auto& stateTransitions = lr0Transitions.at(i);
                if (stateTransitions.count(symStr)) {
                    ui->LR0DFATableWidget->setItem(i, symbolColIndex, new QTableWidgetItem(QString::number(stateTransitions.at(symStr))));
                }
            }
            symbolColIndex++;
        }
    }
    ui->LR0DFATableWidget->resizeRowsToContents();
    ui->LR0DFATableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::displaySLR1Status() {
    if (grammar.isSLR1ConflictFree()) {
        ui->SLRStatusOutput->setPlainText(tr("文法是SLR(1)。"));
    } else {
        QString statusText = tr("文法不是SLR(1)，存在以下冲突:\n\n");
        const auto& conflicts = grammar.getSLRConflictMessages();
        for (const string& conflict : conflicts) {
            statusText += QString::fromStdString(conflict) + "\n\n"; 
        }
        ui->SLRStatusOutput->setPlainText(statusText.trimmed());
    }
}

void MainWindow::displayLR1DFA() {
    const auto& lr1States = grammar.getLR1States();
    const auto& lr1Transitions = grammar.getLR1Transitions();
    const auto& productions = grammar.getProductions();

    if (lr1States.empty() && grammar.getErrorMessage().empty()) {
        return; 
    } else if (lr1States.empty()) {
        return;
    }
    
    QStringList lr1HeaderLabels;
    lr1HeaderLabels << tr("状态ID") << tr("LR(1)项");

    set<string> lr1TransitionSymbolsSet;
    for (const auto& nt : grammar.getNonTerminals()) {
         if (nt != Grammar::AUGMENTED_START_SYMBOL || nt == grammar.getStartSymbol()) { 
            lr1TransitionSymbolsSet.insert(nt);
         }
    }
    for (const auto& t : grammar.getTerminals()) {
        if (t != Grammar::EPSILON) {
            lr1TransitionSymbolsSet.insert(t);
        }
    }

    QStringList lr1SymbolHeaders;
    for(const string& sym : lr1TransitionSymbolsSet) {
        lr1SymbolHeaders << QString::fromStdString(sym);
    }
    lr1HeaderLabels.append(lr1SymbolHeaders);
    ui->LR1DFATableWidget->setColumnCount(lr1HeaderLabels.size());
    ui->LR1DFATableWidget->setHorizontalHeaderLabels(lr1HeaderLabels);

    ui->LR1DFATableWidget->setRowCount(lr1States.size());
    for (size_t i = 0; i < lr1States.size(); ++i) {
        ui->LR1DFATableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        QString lr1ItemsString;
        for (const auto& item : lr1States[i]) {
            lr1ItemsString += QString::fromStdString(item.toString(productions)) + "\n";
        }
        QTableWidgetItem* lr1ItemsCell = new QTableWidgetItem(lr1ItemsString.trimmed());
        ui->LR1DFATableWidget->setItem(i, 1, lr1ItemsCell);

        int lr1SymbolColIndex = 2;
        for (const string& symStr : lr1TransitionSymbolsSet) {
            if (lr1Transitions.count(i)) {
                const auto& stateTrans = lr1Transitions.at(i);
                if (stateTrans.count(symStr)) {
                    ui->LR1DFATableWidget->setItem(i, lr1SymbolColIndex, new QTableWidgetItem(QString::number(stateTrans.at(symStr))));
                }
            }
            lr1SymbolColIndex++;
        }
    }
    ui->LR1DFATableWidget->resizeRowsToContents();
    ui->LR1DFATableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::displayLR1ParsingTableAndConflicts(){
    const auto& lr1ActionTable = grammar.getLR1ActionTable();
    const auto& lr1GotoTable = grammar.getLR1GotoTable();
    const auto& productions = grammar.getProductions();

    if (lr1ActionTable.empty() && grammar.getLR1States().empty() && grammar.getErrorMessage().empty()) {
        return;
    } else if (lr1ActionTable.empty() && grammar.getLR1States().empty()) {
        return;
    } else if (lr1ActionTable.empty() && !grammar.getLR1States().empty()) {
        QString currentText = ui->SLRStatusOutput->toPlainText();
        if (!currentText.isEmpty() && !currentText.endsWith("\n\n")) currentText += "\n\n";
        currentText += tr("注意: LR(1)分析表无法完全构建，尽管LR(1)状态存在。");
        ui->SLRStatusOutput->setPlainText(currentText);
        return;
    }

    QStringList tableHeaders;
    tableHeaders << tr("状态");

    vector<string> actionSymbols;
    for (const auto& t : grammar.getTerminals()) {
        if (t != Grammar::EPSILON) actionSymbols.push_back(t);
    }
    actionSymbols.push_back(Grammar::END_OF_INPUT); 
    sort(actionSymbols.begin(), actionSymbols.end()); 
    for (const string& sym : actionSymbols) {
        tableHeaders << QString::fromStdString(sym);
    }

    vector<string> gotoSymbols;
    for (const auto& nt : grammar.getNonTerminals()) {
        if (nt != Grammar::AUGMENTED_START_SYMBOL && nt != Grammar::EPSILON) {
            gotoSymbols.push_back(nt);
        }
    }
    sort(gotoSymbols.begin(), gotoSymbols.end());
    for (const string& sym : gotoSymbols) {
        tableHeaders << QString::fromStdString(sym);
    }

    ui->LR1AnalyzeTableWidget->setColumnCount(tableHeaders.size());
    ui->LR1AnalyzeTableWidget->setHorizontalHeaderLabels(tableHeaders);
    ui->LR1AnalyzeTableWidget->setRowCount(lr1ActionTable.size());

    for (size_t i = 0; i < lr1ActionTable.size(); ++i) {
        ui->LR1AnalyzeTableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        int currentColumn = 1;
        const auto& actionsForState = lr1ActionTable[i];
        for (const string& terminalSym : actionSymbols) {
            if (actionsForState.count(terminalSym)) {
                ui->LR1AnalyzeTableWidget->setItem(i, currentColumn, 
                    new QTableWidgetItem(QString::fromStdString(actionsForState.at(terminalSym).toString(productions))));
            }
            currentColumn++;
        }
        const auto& gotosForState = lr1GotoTable[i];
        for (const string& nonTerminalSym : gotoSymbols) {
            if (gotosForState.count(nonTerminalSym)) {
                ui->LR1AnalyzeTableWidget->setItem(i, currentColumn, 
                    new QTableWidgetItem(QString::number(gotosForState.at(nonTerminalSym))));
            }
            currentColumn++;
        }
    }
    ui->LR1AnalyzeTableWidget->resizeRowsToContents();
    ui->LR1AnalyzeTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    if (!grammar.isLR1ConflictFree()) {
        QString conflictText = ui->SLRStatusOutput->toPlainText();
        if (!conflictText.isEmpty() && !conflictText.endsWith("\n\n")) {
             conflictText += "\n\n";
        }
        conflictText += tr("LR(1)分析表检测到冲突:\n");
        for (const string& msg : grammar.getLR1TableConflictMessages()) {
            conflictText += QString::fromStdString(msg) + "\n";
        }
        ui->SLRStatusOutput->setPlainText(conflictText.trimmed());
    } else {
        QString currentText = ui->SLRStatusOutput->toPlainText();
        if (grammar.isSLR1ConflictFree()) {
            if (!currentText.contains(tr("文法是SLR(1)。"))) {
                 if (!currentText.isEmpty() && !currentText.endsWith("\n\n")) currentText += "\n\n";
                 currentText += tr("文法是SLR(1)。");
            }
            currentText += tr("\n\n该文法也是LR(1) (LR(1)分析表中无冲突)。");
        } else {
            if (!currentText.contains(tr("不是SLR(1)"))) {
                 if (!currentText.isEmpty() && !currentText.endsWith("\n\n")) currentText += "\n\n";
                 currentText += tr("文法不是SLR(1)。 ");
            }
            currentText += tr("\n\n然而，该文法是LR(1) (LR(1)分析表中无冲突)。");
        }
        ui->SLRStatusOutput->setPlainText(currentText.trimmed());
    }
}

void MainWindow::on_pushButton_analyze_clicked()
{
    clearOutputWidgets();

    QString grammarInput = ui->inputEditor->toPlainText();
    bool parsed = grammar.parseGrammar(grammarInput);
    if (parsed) {
        displayFirstFollowSets();
        displayLR0DFA();
        displaySLR1Status();
        displayLR1DFA();
        displayLR1ParsingTableAndConflicts();
    } else {
        QMessageBox::critical(this, tr("文法错误"), QString::fromStdString(grammar.getErrorMessage()));
        clearOutputWidgets(); 
    }
}
