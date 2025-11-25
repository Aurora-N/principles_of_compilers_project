#ifndef TOASTNOTIFICATION_H
#define TOASTNOTIFICATION_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

enum ToastType {
    SUCCESS, WARNING, ERROR
};

class ToastNotification : public QWidget
{
    Q_OBJECT

public:
    ToastNotification(const QString &message, const ToastType toastType, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        // 设置窗口透明背景和无边框
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        // 创建一个标签，显示消息
        QLabel *label = new QLabel(message, this);
        switch (toastType) {
        case SUCCESS:
            label->setStyleSheet("font-size: 16px; color: white; border-radius: 5px; background-color: #42d392;");
            break;
        case WARNING:
            label->setStyleSheet("font-size: 16px; color: white; border-radius: 5px; background-color: #ffa104;");
            break;
        case ERROR:
            label->setStyleSheet("font-size: 16px; color: white; border-radius: 5px; background-color: #f56c6c;");
            break;
        default:
            label->setStyleSheet("font-size: 16px; color: white; border-radius: 5px; background-color: #c8c9cc;");
            break;
        }
        label->setAlignment(Qt::AlignCenter);  // 设置文本居中对齐

        // 设置布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(label);
        setLayout(layout);

        resize(240, 60);

        // 定时器在2.5秒后自动关闭窗口
        QTimer::singleShot(2500, this, &QWidget::close);

    }

    void showAtTopCenter(QWidget *parent)
    {
        // 计算主窗口的水平居中位置
        int x = parent->x() + (parent->width() - width()) / 2;
        int y = parent->y() + 10;
        move(x, y);
        show();
    }

    void showAt(const QPoint &pos)
    {
        move(pos);
        show();
    }
};
#endif // TOASTNOTIFICATION_H
