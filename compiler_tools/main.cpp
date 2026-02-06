#include "lr1mainwindow.h"
#include "regexmainwindow.h"

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFont>

class AppSelectorDialog : public QDialog
{
public:
    enum AppChoice {
        None,
        RegexConverter,
        LR1Analyzer
    };

    AppSelectorDialog(QWidget *parent = nullptr) : QDialog(parent), choice(None)
    {
        setWindowTitle("编译器工具集");
        setFixedSize(450, 300);
        setStyleSheet(R"(
            QDialog {
                background-color: #f5f7fa;
            }
            QLabel#titleLabel {
                font-size: 18px;
                font-weight: bold;
                color: #2c3e50;
            }
            QLabel#subtitleLabel {
                font-size: 12px;
                color: #7f8c8d;
            }
            QPushButton {
                background-color: #ffffff;
                border: 2px solid #e0e0e0;
                border-radius: 10px;
                padding: 15px;
                font-size: 14px;
                color: #333333;
                text-align: left;
            }
            QPushButton:hover {
                border-color: #0060DF;
                background-color: #f0f7ff;
            }
            QPushButton:pressed {
                background-color: #e0efff;
            }
        )");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(30, 25, 30, 25);

        // 标题
        QLabel *titleLabel = new QLabel("编译器工具集");
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        // 副标题
        QLabel *subtitleLabel = new QLabel("请选择要使用的工具");
        subtitleLabel->setObjectName("subtitleLabel");
        subtitleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(subtitleLabel);

        mainLayout->addSpacing(10);

        // 正则转换器按钮
        QPushButton *regexBtn = new QPushButton();
        regexBtn->setFixedHeight(70);
        QVBoxLayout *regexLayout = new QVBoxLayout(regexBtn);
        regexLayout->setContentsMargins(15, 10, 15, 10);
        QLabel *regexTitle = new QLabel("🔤 正则表达式转换器");
        regexTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; background: transparent;");
        QLabel *regexDesc = new QLabel("正则表达式 → NFA → DFA → 最小化DFA → 词法分析代码");
        regexDesc->setStyleSheet("font-size: 11px; color: #7f8c8d; background: transparent;");
        regexLayout->addWidget(regexTitle);
        regexLayout->addWidget(regexDesc);
        mainLayout->addWidget(regexBtn);

        // LR1分析器按钮
        QPushButton *lr1Btn = new QPushButton();
        lr1Btn->setFixedHeight(70);
        QVBoxLayout *lr1Layout = new QVBoxLayout(lr1Btn);
        lr1Layout->setContentsMargins(15, 10, 15, 10);
        QLabel *lr1Title = new QLabel("📊 LR(1) 语法分析器");
        lr1Title->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; background: transparent;");
        QLabel *lr1Desc = new QLabel("First/Follow集 → LR(0)/LR(1) DFA → 分析表 → 语法树");
        lr1Desc->setStyleSheet("font-size: 11px; color: #7f8c8d; background: transparent;");
        lr1Layout->addWidget(lr1Title);
        lr1Layout->addWidget(lr1Desc);
        mainLayout->addWidget(lr1Btn);

        mainLayout->addStretch();

        // 连接信号
        connect(regexBtn, &QPushButton::clicked, this, [this]() {
            choice = RegexConverter;
            accept();
        });

        connect(lr1Btn, &QPushButton::clicked, this, [this]() {
            choice = LR1Analyzer;
            accept();
        });
    }

    AppChoice getChoice() const { return choice; }

private:
    AppChoice choice;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 显示选择对话框
    AppSelectorDialog selector;
    if (selector.exec() != QDialog::Accepted) {
        return 0;  // 用户关闭了对话框，退出程序
    }

    // 根据用户选择打开对应的应用
    switch (selector.getChoice()) {
    case AppSelectorDialog::RegexConverter: {
        RegexMainWindow *regexWindow = new RegexMainWindow();
        regexWindow->setAttribute(Qt::WA_DeleteOnClose);
        regexWindow->show();
        break;
    }
    case AppSelectorDialog::LR1Analyzer: {
        LR1MainWindow *lr1Window = new LR1MainWindow();
        lr1Window->setAttribute(Qt::WA_DeleteOnClose);
        lr1Window->show();
        break;
    }
    default:
        return 0;
    }

    return a.exec();
}
