#ifndef LR1MAINWINDOW_H
#define LR1MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "grammar.h"
#include "qtreewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LR1MainWindow;
}
QT_END_NAMESPACE

class LR1MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    LR1MainWindow(QWidget *parent = nullptr);
    ~LR1MainWindow();

private slots:
    void on_pushButton_save_clicked();
    void on_pushButton_open_clicked();
    void on_pushButton_analyze_clicked();

    void on_pushButton_select_source_clicked();

    void on_pushButton_gammar_analyze_clicked();

private:
    Ui::LR1MainWindow *ui;
    Grammar grammar;

    void clearOutputWidgets();
    void clearLR1AnalysisOutputWidgets();
    void displayFirstFollowSets();
    void displayLR0DFA();
    void displaySLR1Status();
    void displayLR1DFA();
    void displayLR1ParsingTableAndConflicts();
    void displaySyntaxTree(const Grammar::SyntaxTreeNode* node, QTreeWidgetItem* parent);
};
#endif // LR1MAINWINDOW_H
