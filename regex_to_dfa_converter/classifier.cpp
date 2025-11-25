#include <sstream>
#include <string>
#include <vector>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include "classifier.h"

using namespace std;

string classifier(string input_text) {
    istringstream stream(input_text);
    ostringstream output;
    string line;

    QDir appDir(QCoreApplication::applicationDirPath());
    QDir tempDir(appDir.filePath("temp"));
    // 读取编译生成的词法分析程序
    QString binBaseName = "generated_lexer";
#ifdef Q_OS_WIN
    QString binPath = QDir::toNativeSeparators(tempDir.filePath(binBaseName + ".exe"));
#else
    QString binPath = QDir::toNativeSeparators(tempDir.filePath(binBaseName));
#endif
    if (!QFile::exists(binPath)) {
        output << "未找到已编译的词法程序，请先点击“正则表达式转换”tab下的“词法分析代码”进行编译\n";
        return output.str();
    }

    while (std::getline(stream, line)) {
        // 去除行首空白
        size_t start = line.find_first_not_of(" \t");
        if (start == string::npos) continue;
        line = line.substr(start);

        // 输出原始行
        output << line << "\n";

        QProcess runner;
        runner.start(binPath);
        if (!runner.waitForStarted(5000)) {
            output << "程序启动失败\n";
            break;
        }
        QByteArray inputBytes(line.c_str());
        inputBytes.append('\n');
        runner.write(inputBytes);
        runner.closeWriteChannel();
        if (!runner.waitForFinished(10000)) {
            output << "程序运行超时\n";
            break;
        }
        QByteArray result = runner.readAllStandardOutput();
        QByteArray err = runner.readAllStandardError();
        if (!err.isEmpty()) {
            output << "运行错误: " << err.constData() << "\n";
        } else {
            // 将生成代码的输出作为分词结果附加
            std::string res = result.constData();
            if (!res.empty() && res.back() == '\n') res.pop_back();
            output << "    " << res << "\n\n";
        }
    }

    return output.str();
}
