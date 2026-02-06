#ifndef REGEXMAINWINDOW_H
#define REGEXMAINWINDOW_H
#include "regex.h"
#include "toastnotification.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class RegexMainWindow;
}
QT_END_NAMESPACE

class RegexMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    RegexMainWindow(QWidget *parent = nullptr);
    ~RegexMainWindow();

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
    Ui::RegexMainWindow *ui;

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
#endif // REGEXMAINWINDOW_H
