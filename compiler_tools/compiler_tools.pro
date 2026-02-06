QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = compiler_tools

SOURCES += \
    main.cpp \
    lr1mainwindow.cpp \
    regexmainwindow.cpp \
    lr1_analyser/grammar.cpp \
    regex_converter/classifier.cpp \
    regex_converter/codegenerate.cpp \
    regex_converter/nfa.cpp \
    regex_converter/regex.cpp

HEADERS += \
    lr1mainwindow.h \
    regexmainwindow.h \
    lr1_analyser/grammar.h \
    regex_converter/classifier.h \
    regex_converter/codegenerate.h \
    regex_converter/nfa.h \
    regex_converter/regex.h \
    regex_converter/toastnotification.h

FORMS += \
    lr1mainwindow.ui \
    regexmainwindow.ui

INCLUDEPATH += \
    lr1_analyser \
    regex_converter

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
