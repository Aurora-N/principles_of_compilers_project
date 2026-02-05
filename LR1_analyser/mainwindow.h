#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "grammar.h"
#include "qtreewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_save_clicked();
    void on_pushButton_open_clicked();
    void on_pushButton_analyze_clicked();

    void on_pushButton_select_source_clicked();

    void on_pushButton_gammar_analyze_clicked();

private:
    Ui::MainWindow *ui;
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
#endif // MAINWINDOW_H
