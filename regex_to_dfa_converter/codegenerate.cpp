#include "codegenerate.h"
#include "qdebug.h"
#include <QSet>
#include <QMap>

QString generateIdPredicate(const Regex &idRegex)
{
    // 生成用于判断某标识符字符类的函数代码字符串：
    // - 若字符集合等价于所有英文字母([A-Za-z])，返回使用 isalpha 的函数
    // - 若字符集合等价于所有数字([0-9])，返回使用 isdigit 的函数
    // - 否则返回逐字符枚举的布尔表达式函数
    // 说明：此函数依赖前置的范围展开逻辑（expandRange），即 [a-b] 已被转换为 (a|b|...)。
    QString func;
    QString idName = QString::fromStdString(idRegex.id);
    QSet<QChar> chars;
    std::string s = idRegex.content;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            chars.insert(QChar(s[i + 1]));
            i++;
            continue;
        }
        if (c == '(' || c == ')' || c == '|' || c == '*' || c == '&' || c == '#')
            continue;
        chars.insert(QChar(c));
    }
    // 检查是否恰好为 10 个数字字符
    bool isDigits = (chars.size() == 10);
    if (isDigits)
    {
        for (char d = '0'; d <= '9'; ++d)
        {
            if (!chars.contains(QChar(d)))
            {
                isDigits = false;
                break;
            }
        }
    }
    // 检查是否恰好为 52 个英文字母字符（A-Z 与 a-z）
    bool isLetters = (chars.size() == 52);
    if (isLetters)
    {
        for (char a = 'A'; a <= 'Z'; ++a)
        {
            if (!chars.contains(QChar(a)))
            {
                isLetters = false;
                break;
            }
        }
        if (isLetters)
        {
            for (char a = 'a'; a <= 'z'; ++a)
            {
                if (!chars.contains(QChar(a)))
                {
                    isLetters = false;
                    break;
                }
            }
        }
    }
    if (isLetters)
        return QString("bool is_%1(char ch){ return isalpha(ch); }\n").arg(idName);
    if (isDigits)
        return QString("bool is_%1(char ch){ return isdigit(ch); }\n").arg(idName);
    // 不是标准全集时，按位枚举生成判断函数
    QStringList parts;
    for (auto it = chars.begin(); it != chars.end(); ++it) {
        if (*it == '\\')
            parts << "ch == '\\\\'";
        else if (*it == '\'')
            parts << "ch == '\\''";
        else
            parts << QString("ch == '%1'").arg(*it);
    }
    qDebug() << parts.join(" || ") << "at generateIdPredicate()";
    return QString("bool is_%1(char ch){ return %2; }\n").arg(idName, parts.join(" || "));
}

const Regex *findIdRegex(const QString &id, QList<Regex> regexs)
{
    // 在当前已解析的正则列表中查找名称为 id 的标识符定义
    for (const auto &r : regexs)
    {
        if (QString::fromStdString(r.id) == id)
            return &r;
    }
    return nullptr;
}

QString generateLexerCodeFragmentForRegex(const Regex &regex, QList<Regex> regexs)
{
    // 将最小化 DFA 转换为词法分析代码字符串
    // 改为生成 match_XXX 函数，返回匹配长度，若无匹配返回 -1
    QString code;

    // 起始态与接受态集合
    int start = regex.minDfa.startState.empty() ? 0 : *regex.minDfa.startState.rbegin();
    QSet<int> accepts;
    for (const auto &a : regex.minDfa.acceptStates)
        accepts.insert(*a.rbegin());

    // 将转移按 from->to 聚合符号
    QMap<int, QMap<int, QStringList>> transitions;
    for (const auto &t : regex.minDfa.transitions)
    {
        int from = *t.from.rbegin();
        int to = *t.to.rbegin();
        QString sym = QString::fromStdString(t.symbol);
        transitions[from][to].append(sym);
    }
    // 预构建字母/数字全集
    QSet<QChar> lettersSet;
    for (char a = 'A'; a <= 'Z'; ++a)
        lettersSet.insert(QChar(a));
    for (char a = 'a'; a <= 'z'; ++a)
        lettersSet.insert(QChar(a));
    QSet<QChar> digitsSet;
    for (char d = '0'; d <= '9'; ++d)
        digitsSet.insert(QChar(d));

    // 生成 match 函数名
    QString funcName = QString::fromStdString(regex.id.empty() ? "match_Token" : ("match_" + regex.id));
    code += QString("int %1(const string& s, int p) {\n").arg(funcName);
    code += QString("  int state = %1; int cur = p; int lastAcceptLen = -1;\n").arg(start);
    code += "  bool done = false; char ch;\n";
    
    // 检查起始状态是否为接受态
    if (accepts.contains(start)) {
        code += "  lastAcceptLen = 0;\n";
    }

    code += "  while (!done && cur < s.length()) {\n";
    code += "    ch = s[cur];\n";
    code += "    switch (state) {\n";

    QList<int> fromStates = transitions.keys();
    std::sort(fromStates.begin(), fromStates.end());
    for (int from : fromStates)
    {
        code += QString("      case %1:\n").arg(from);
        code += "      { bool moved = false;\n";
        QMap<int, QStringList> outs = transitions[from];
        QList<int> tos = outs.keys();
        std::sort(tos.begin(), tos.end());
        for (int to : tos)
        {
            QStringList syms = outs[to];
            QStringList conds;
            QSet<QChar> charSet;
            for (const QString &s : syms)
            {
                const Regex *def = findIdRegex(s, regexs);
                if (def)
                {
                    conds << QString("is_%1(ch)").arg(s);
                }
                else if (s.size() == 1)
                {
                    charSet.insert(s[0]);
                }
            }
            if (!charSet.isEmpty())
            {
                bool isLettersGroup = (charSet.size() == lettersSet.size());
                if (isLettersGroup)
                {
                    for (auto it = lettersSet.begin(); it != lettersSet.end(); ++it)
                    {
                        if (!charSet.contains(*it)) { isLettersGroup = false; break; }
                    }
                }
                bool isDigitsGroup = (charSet.size() == digitsSet.size());
                if (isDigitsGroup)
                {
                    for (auto it = digitsSet.begin(); it != digitsSet.end(); ++it)
                    {
                        if (!charSet.contains(*it)) { isDigitsGroup = false; break; }
                    }
                }
                if (isLettersGroup)
                    conds << "isalpha(ch)";
                else if (isDigitsGroup)
                    conds << "isdigit(ch)";
                else
                {
                    for (auto it = charSet.begin(); it != charSet.end(); ++it) {
                        if (*it == '\\')
                            conds << "ch == '\\\\'";
                        else if (*it == '\'')
                            conds << "ch == '\\''";
                        else
                            conds << QString("ch == '%1'").arg(*it);
                    }
                }
            }
            
            // 构造接受态检查
            QString acceptUpdate = "";
            if (accepts.contains(to)) {
                acceptUpdate = " lastAcceptLen = cur - p; ";
            }
            
            code += QString("        if (%1) { state = %2; cur++; moved = true;%3 }\n").arg(conds.join(" || ")).arg(to).arg(acceptUpdate);
        }
        code += "        if (!moved) done = true;\n        break; }\n";
    }

    code += "      default: done = true; break;\n";
    code += "    }\n  }\n";
    code += "  return lastAcceptLen;\n";
    code += "}\n";

    return code;
}

QString generateLexerCode(QList<Regex> regexs)
{
    // 统一生成完整的词法分析器代码：
    // 1) 标识符判定函数 is_xxx
    // 2) 每个正则的匹配函数 match_xxx
    // 3) 主函数：行缓冲，行内分词，最长匹配
    QString code;
    code += "#include <iostream>\n#include <cctype>\n#include <string>\n#include <vector>\nusing namespace std;\n";

    // 1) 统一生成 is_xxx 判定函数
    for (const auto &regex : regexs) {
        if (!regex.id.empty()) {
            code += generateIdPredicate(regex);
        }
    }

    // 2) 生成每个正则的匹配函数
    for (const auto &regex : regexs) {
        code += generateLexerCodeFragmentForRegex(regex, regexs);
    }

    // 3) 主函数
    code += "int main(){\n";
    code += "  string line;\n";
    code += "  int lineNum = 0;\n";
    code += "  while (getline(cin, line)) {\n";
    code += "    lineNum++;\n";
    code += "    int pos = 0;\n";
    code += "    int len = line.length();\n";
    code += "    while (pos < len) {\n";
    code += "      if (isspace(line[pos])) { pos++; continue; }\n";
    code += "      int bestLen = -1;\n";
    code += "      string bestType = \"\";\n";
    
    // 尝试所有 regex
    for (const auto &regex : regexs) {
        QString funcName = QString::fromStdString(regex.id.empty() ? "match_Token" : ("match_" + regex.id));
        QString typeName = QString::fromStdString(regex.id.empty() ? regex.rawRegex : regex.id);
        code += QString("      {\n");
        code += QString("        int l = %1(line, pos);\n").arg(funcName);
        code += QString("        if (l > bestLen) { bestLen = l; bestType = \"%1\"; }\n").arg(typeName);
        code += QString("      }\n");
    }

    code += "      if (bestLen > 0) {\n";
    code += "        cout << \"Line \" << lineNum << \", Col \" << pos+1 << \": <\" << bestType << \", \" << line.substr(pos, bestLen) << \">\" << endl;\n";
    code += "        pos += bestLen;\n";
    code += "      } else {\n";
    code += "        cout << \"Error at line \" << lineNum << \", col \" << pos+1 << \": \" << line[pos] << endl;\n";
    code += "        pos++;\n";
    code += "      }\n";
    code += "    }\n";
    code += "  }\n";
    code += "  return 0;\n}";
    return code;
}
