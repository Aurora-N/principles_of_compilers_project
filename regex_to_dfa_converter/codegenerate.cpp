#include "codegenerate.h"
#include "qdebug.h"
#include <QSet>
#include <QMap>

/**
 * 解析 rawContent 中的范围格式
 * 例如: "[a-e]" 返回 [{'a','e'}]
 * 例如: "[a-zA-Z]" 返回 [{'a','z'}, {'A','Z'}]
 * 例如: "[0-9]" 返回 [{'0','9'}]
 * 如果不是纯范围格式或包含单独字符，返回空列表
 */
QList<QPair<QChar, QChar>> parseRanges(const QString& rawContent) {
    QList<QPair<QChar, QChar>> ranges;
    
    QString content = rawContent;

    if (!content.isEmpty() && content[0] == '=') {
        content = content.sliced(1);
    }

    // 检查是否是 [xxx] 格式
    if (!content.startsWith('[') || !content.endsWith(']')) {
        return ranges;
    }
    
    QString inner = content.mid(1, content.length() - 2);
    int i = 0;
    while (i < inner.length()) {
        if (i + 2 < inner.length() && inner[i + 1] == '-') {
            // 范围格式 a-z
            ranges.append(qMakePair(inner[i], inner[i + 2]));
            i += 3;
        } else {
            // 不是纯范围格式（包含单独字符），返回空
            return QList<QPair<QChar, QChar>>();
        }
    }
    
    return ranges;
}

/**
 * 根据范围列表生成条件表达式
 * 例如: [{'a','e'}] -> "(c >= 'a' && c <= 'e')"
 * 例如: [{'a','z'}, {'A','Z'}] -> "(c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')"
 */
QString generateRangeCondition(const QList<QPair<QChar, QChar>>& ranges) {
    QStringList conditions;
    for (const auto& range : ranges) {
        QChar start = range.first;
        QChar end = range.second;
        if (start == end) {
            // 单字符范围
            conditions.append(QString("c == '%1'").arg(start));
        } else {
            conditions.append(QString("(c >= '%1' && c <= '%2')").arg(start).arg(end));
        }
    }
    return conditions.join(" || ");
}

/**
 * 从正则表达式内容中提取字符集合
 * 例如: (0|1|2|3|4|5|6|7|8|9) -> {'0','1',...,'9'}
 * 例如: (a|b|...|z|A|B|...|Z) -> {'a',...,'z','A',...,'Z'}
 */
QSet<QChar> extractCharSet(const QString& content) {
    QSet<QChar> chars;
    for (int i = 0; i < content.length(); i++) {
        QChar c = content[i];
        // 跳过操作符和控制字符
        if (c == '(' || c == ')' || c == '|' || c == '*' || c == '&' || c == '#') {
            continue;
        }
        // 处理转义字符
        if (c == '\\' && i + 1 < content.length()) {
            chars.insert(content[i + 1]);
            i++;
            continue;
        }
        chars.insert(c);
    }
    return chars;
}

/**
 * 检查字符集是否等价于 [0-9]
 */
bool isDigitSet(const QSet<QChar>& chars) {
    if (chars.size() != 10) return false;
    for (char c = '0'; c <= '9'; c++) {
        if (!chars.contains(QChar(c))) return false;
    }
    return true;
}

/**
 * 检查字符集是否等价于 [a-zA-Z]
 */
bool isLetterSet(const QSet<QChar>& chars) {
    if (chars.size() != 52) return false;
    for (char c = 'a'; c <= 'z'; c++) {
        if (!chars.contains(QChar(c))) return false;
    }
    for (char c = 'A'; c <= 'Z'; c++) {
        if (!chars.contains(QChar(c))) return false;
    }
    return true;
}

/**
 * 生成完整的词法分析器代码
 * 基于最小化DFA，使用最长匹配原则
 */
QString generateLexerCode(QList<Regex> regexs)
{
    QString code;

    // ===================== 头文件和全局变量 =====================
    code += "#include <iostream>\n";
    code += "#include <string>\n";
    code += "#include <vector>\n";
    code += "#include <cctype>\n";
    code += "using namespace std;\n\n";

    code += "// 全局变量\n";
    code += "string buffer;\n";
    code += "int pos = 0;\n";
    code += "char ch;\n\n";

    code += "// 获取下一个字符\n";
    code += "char getNext() {\n";
    code += "    if (pos < (int)buffer.size()) return buffer[pos++];\n";
    code += "    return '\\0';\n";
    code += "}\n\n";

    code += "// 回退一个字符\n";
    code += "void ungetChar() {\n";
    code += "    if (pos > 0) pos--;\n";
    code += "}\n\n";

    // ===================== 构建正则名称到Regex的映射 =====================
    QMap<QString, const Regex*> regexMap;
    for (const Regex& regex : regexs) {
        if (!regex.id.empty()) {
            regexMap[QString::fromStdString(regex.id)] = &regex;
        }
    }

    // ===================== 收集所有符号和标识符 =====================
    QSet<QString> allSymbols;
    QSet<QString> identifierSymbols;  // 标识符类型的符号（如 letter, digit）
    QSet<QString> generatedPredicates;  // 记录已生成的判定函数
    
    for (const Regex& regex : regexs) {
        for (const DFATransition& t : regex.minDfa.transitions) {
            QString sym = QString::fromStdString(t.symbol);
            allSymbols.insert(sym);
            // 只有当符号是一个已定义的正则标识符（在regexMap中定义）时才添加
            // 单字符符号（如 '.', '+' 等）不应被视为标识符符号
            if (regexMap.contains(sym)) {
                identifierSymbols.insert(sym);
            }
        }
    }

    // ===================== 生成字符类判定函数 =====================
    code += "// 字符类判定函数\n";
    
    for (const QString& idSym : identifierSymbols) {
        const Regex* idRegex = regexMap.value(idSym, nullptr);
        if (!idRegex) continue;
        
        QString content = QString::fromStdString(idRegex->content);
        QString rawContent = QString::fromStdString(idRegex->rawContent);
        QSet<QChar> charSet = extractCharSet(content);
        
        // 跳过空字符集或复杂正则（包含引用其他标识符的）
        if (charSet.isEmpty()) continue;
        
        generatedPredicates.insert(idSym);
        code += "bool is_" + idSym + "(char c) {\n";
        
        // 首先尝试从 rawContent 解析范围格式
        QList<QPair<QChar, QChar>> ranges = parseRanges(rawContent);
        if (!ranges.isEmpty()) {
            // 使用范围表达式
            code += "    return " + generateRangeCondition(ranges) + ";\n";
        } else {
            // 回退到字符枚举判断
            QStringList conditions;
            for (const QChar& c : charSet) {
                if (c == '\'') {
                    conditions.append("c == '\\''");
                } else if (c == '\\') {
                    conditions.append("c == '\\\\'");
                } else {
                    conditions.append(QString("c == '%1'").arg(c));
                }
            }
            if (conditions.isEmpty()) {
                code += "    return false;\n";
            } else {
                code += "    return " + conditions.join(" || ") + ";\n";
            }
        }
        code += "}\n\n";
    }

    // ===================== 为每个需要的正则生成匹配函数 =====================
    // 收集所有需要生成匹配函数的正则
    // 只有以 '_' 开头的标识符才生成 match 函数，其他的只生成 is 判定函数
    QList<const Regex*> matchableRegexs;
    for (const Regex& regex : regexs) {
        if (regex.id.empty()) continue;
        QString regexId = QString::fromStdString(regex.id);
        // 只有以 '_' 开头的正则才生成 match 函数
        if (!regexId.startsWith('_')) {
            continue;
        }
        matchableRegexs.append(&regex);
    }

    // 生成每个正则的匹配函数
    for (const Regex* regex : matchableRegexs) {
        QString funcName = QString::fromStdString(regex->id);
        
        code += "// 匹配函数: " + funcName + "\n";
        code += "// 正则表达式: " + QString::fromStdString(regex->rawRegex) + "\n";
        code += "int match_" + funcName + "(int startPos) {\n";
        code += "    int savePos = pos;\n";
        code += "    pos = startPos;\n";
        
        // 获取开始状态
        int startState = *regex->minDfa.startState.rbegin();
        code += "    int state = " + QString::number(startState) + ";\n";
        code += "    int lastAcceptPos = -1;\n";
        code += "    ch = getNext();\n\n";

        // 收集接受状态
        QSet<int> acceptStates;
        for (const auto& accept : regex->minDfa.acceptStates) {
            acceptStates.insert(*accept.rbegin());
        }

        // 收集所有状态
        QSet<int> allStates;
        allStates.insert(startState);
        for (const DFATransition& t : regex->minDfa.transitions) {
            allStates.insert(*t.from.rbegin());
            allStates.insert(*t.to.rbegin());
        }

        // 检查初始状态是否为接受状态
        if (acceptStates.contains(startState)) {
            code += "    lastAcceptPos = startPos;\n";
        }

        code += "    while (ch != '\\0') {\n";
        code += "        bool moved = false;\n";
        code += "        switch (state) {\n";

        // 为每个状态生成case
        for (int state : allStates) {
            code += "            case " + QString::number(state) + ":\n";

            // 收集该状态的所有转移，按目标状态分组
            QMap<int, QStringList> toStateSymbols;  // toState -> symbols
            for (const DFATransition& t : regex->minDfa.transitions) {
                if (*t.from.rbegin() == state) {
                    toStateSymbols[*t.to.rbegin()].append(QString::fromStdString(t.symbol));
                }
            }

            if (toStateSymbols.isEmpty()) {
                code += "                break;\n";
            } else {
                bool firstIf = true;
                for (auto it = toStateSymbols.begin(); it != toStateSymbols.end(); ++it) {
                    int toState = it.key();
                    QStringList symbols = it.value();

                    // 生成条件表达式
                    QStringList conditions;
                    for (const QString& sym : symbols) {
                        if (generatedPredicates.contains(sym)) {
                            // 使用字符类判定函数
                            conditions.append("is_" + sym + "(ch)");
                        } else if (sym.length() == 1) {
                            // 单字符
                            QChar c = sym[0];
                            if (c == '\'') {
                                conditions.append("ch == '\\''");
                            } else if (c == '\\') {
                                conditions.append("ch == '\\\\'");
                            } else if (c == '\n') {
                                conditions.append("ch == '\\n'");
                            } else if (c == '\t') {
                                conditions.append("ch == '\\t'");
                            } else {
                                conditions.append("ch == '" + sym + "'");
                            }
                        } else {
                            // 多字符字面量（不应该出现）
                            qDebug() << "Warning: unexpected multi-char symbol:" << sym;
                        }
                    }

                    QString condStr = conditions.join(" || ");
                    QString prefix = firstIf ? "if" : "else if";
                    firstIf = false;

                    code += "                " + prefix + " (" + condStr + ") {\n";
                    code += "                    state = " + QString::number(toState) + ";\n";
                    code += "                    moved = true;\n";
                    if (acceptStates.contains(toState)) {
                        code += "                    lastAcceptPos = pos;\n";
                    }
                    code += "                }\n";
                }
                code += "                break;\n";
            }
        }

        code += "            default:\n";
        code += "                break;\n";
        code += "        }\n";
        code += "        if (!moved) break;\n";
        code += "        ch = getNext();\n";
        code += "    }\n\n";

        code += "    pos = savePos;\n";
        code += "    if (lastAcceptPos > startPos) return lastAcceptPos - startPos;\n";
        code += "    if (lastAcceptPos == startPos) return 0;\n";
        code += "    return -1;\n";
        code += "}\n\n";
    }

    // ===================== 生成Token结构和getNextToken函数 =====================
    code += "// Token结构\n";
    code += "struct Token {\n";
    code += "    string type;\n";
    code += "    string value;\n";
    code += "};\n\n";

    code += "// 使用最长匹配原则获取下一个Token\n";
    code += "Token getNextToken() {\n";
    code += "    Token token;\n";
    code += "    token.type = \"ERROR\";\n";
    code += "    token.value = \"\";\n\n";
    code += "    // 跳过空白字符\n";
    code += "    while (pos < (int)buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\\t' || buffer[pos] == '\\n' || buffer[pos] == '\\r')) {\n";
    code += "        pos++;\n";
    code += "    }\n\n";
    code += "    if (pos >= (int)buffer.size()) {\n";
    code += "        token.type = \"EOF\";\n";
    code += "        return token;\n";
    code += "    }\n\n";
    code += "    int startPos = pos;\n";
    code += "    int maxLen = -1;\n";
    code += "    string matchedType = \"\";\n\n";

    // 对每种token类型尝试匹配
    // 关键字应该优先于标识符（当匹配长度相同时）
    for (const Regex* regex : matchableRegexs) {
        QString typeName = QString::fromStdString(regex->id);
        code += "    // 尝试匹配 " + typeName + "\n";
        code += "    {\n";
        code += "        int len = match_" + typeName + "(startPos);\n";
        // 最长匹配；相同长度时，关键字优先于identifier
        code += "        if (len > maxLen || (len == maxLen && len > 0 && matchedType == \"identifier\")) {\n";
        code += "            maxLen = len;\n";
        code += "            matchedType = \"" + typeName + "\";\n";
        code += "        }\n";
        code += "    }\n\n";
    }

    code += "    if (maxLen > 0) {\n";
    code += "        token.type = matchedType;\n";
    code += "        token.value = buffer.substr(startPos, maxLen);\n";
    code += "        pos = startPos + maxLen;\n";
    code += "    } else if (maxLen == 0) {\n";
    code += "        token.type = \"ERROR\";\n";
    code += "        token.value = buffer.substr(startPos, 1);\n";
    code += "        pos = startPos + 1;\n";
    code += "    } else {\n";
    code += "        token.type = \"ERROR\";\n";
    code += "        token.value = buffer.substr(startPos, 1);\n";
    code += "        pos = startPos + 1;\n";
    code += "    }\n\n";
    code += "    return token;\n";
    code += "}\n\n";

    // ===================== 生成main函数 =====================
    code += "// 主函数\n";
    code += "int main() {\n";
    code += "    // 读取一行输入\n";
    code += "    getline(cin, buffer);\n\n";
    code += "    // 词法分析\n";
    code += "    bool first = true;\n";
    code += "    while (pos < (int)buffer.size()) {\n";
    code += "        Token token = getNextToken();\n";
    code += "        if (token.type == \"EOF\") break;\n";
    code += "        if (!first) cout << \" \";\n";
    code += "        first = false;\n";
    code += "        cout << \"【\" << (token.type[0] == '_' ? token.type.substr(1) : token.type) << \" : \" << token.value << \"】\";\n";
    code += "    }\n";
    code += "    cout << endl;\n\n";
    code += "    return 0;\n";
    code += "}\n";

    return code;
}
