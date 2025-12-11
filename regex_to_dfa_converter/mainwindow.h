#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "regex.h"
#include "toastnotification.h"
#include <QMainWindow>

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
    void on_pushButton_do_convert_clicked();

    void on_pushButton_import_regex_clicked();

    void on_pushButton_show_nfa_clicked();

    void on_pushButton_show_dfa_clicked();

    void on_pushButton_show_mdfa_clicked();

    void on_pushButton_show_code_clicked();

    void on_pushButton_do_lex_clicked();

    void on_pushButton_import_lex_source_clicked();

private:
    Ui::MainWindow *ui;

    QList<Regex> regexs;

    QString inputContent;       // 输入框中的内容
    QString inputContentLex;    // 词法分析输入框的内容
    /** 获取文本中的内容，并将内容返回 */
    QString getFileContent(QString filePath);
    /** 创建并显示一个Toast通知 */
    void createToastNotification(QString content, ToastType toastType);
    /** 编译词法分析代码 */
    void compileLexCode();
};
#endif // MAINWINDOW_H
