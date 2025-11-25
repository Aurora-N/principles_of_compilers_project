#include "codegenerate.h"
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
    for (auto it = chars.begin(); it != chars.end(); ++it)
        parts << QString("ch == '%1'").arg(*it);
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

QString generateLexerCodeForRegex(const Regex &regex, QList<Regex> regexs)
{
    // 将最小化 DFA 转换为词法分析代码字符串
    // 生成内容包括：必要头文件、输入接口占位、标识符判断函数、
    // 基于 switch(state) 的状态机扫描、接受态判断与输出。
    QString code;
    code += "#include <iostream>\n#include <cctype>\n#include <string>\nusing namespace std;\n";
    code += "char buffer[1024]; int pos = 0; char ch;\n";
    code += "char GetNext(){ return buffer[pos++]; }\n";

    // 收集当前正则使用到的标识符，并生成对应的判断函数
    QSet<QString> usedIds;
    for (const auto &t : regex.minDfa.transitions)
    {
        QString sym = QString::fromStdString(t.symbol);
        if (findIdRegex(sym, regexs))
            usedIds.insert(sym);
    }
    for (const auto &id : usedIds)
    {
        const Regex *def = findIdRegex(id, regexs);
        if (def)
            code += generateIdPredicate(*def);
    }

    // 起始态与接受态集合（状态编号为单元素集合的值）
    int start = regex.minDfa.startState.empty() ? 0 : *regex.minDfa.startState.rbegin();
    QSet<int> accepts;
    for (const auto &a : regex.minDfa.acceptStates)
        accepts.insert(*a.rbegin());

    // 将转移按 from->to 聚合符号，便于合并条件并压缩
    QMap<int, QMap<int, QStringList>> transitions;
    for (const auto &t : regex.minDfa.transitions)
    {
        int from = *t.from.rbegin();
        int to = *t.to.rbegin();
        QString sym = QString::fromStdString(t.symbol);
        transitions[from][to].append(sym);
    }
    // 预构建字母/数字全集，用于识别并压缩为 isalpha/isdigit
    QSet<QChar> lettersSet;
    for (char a = 'A'; a <= 'Z'; ++a)
        lettersSet.insert(QChar(a));
    for (char a = 'a'; a <= 'z'; ++a)
        lettersSet.insert(QChar(a));
    QSet<QChar> digitsSet;
    for (char d = '0'; d <= '9'; ++d)
        digitsSet.insert(QChar(d));

    // 生成核心扫描函数名：有 id 则使用 GetToken_<id>，否则为 GetToken
    QString funcName = QString::fromStdString(regex.id.empty() ? "GetToken" : ("GetToken_" + regex.id));
    code += QString("void %1(){\n").arg(funcName);
    code += QString("  int state = %1; bool done = false; ch = GetNext();\n").arg(start);
    code += "  while (!done) {\n    switch (state) {\n";

    QList<int> fromStates = transitions.keys();
    std::sort(fromStates.begin(), fromStates.end());
    for (int from : fromStates)
    {
        // 针对单个状态，合并其外出边的条件并生成 if 分支
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
            // 将完整的并集压缩为 isalpha/isdigit；否则逐字符比较或使用 is_<id>
            if (!charSet.isEmpty())
            {
                bool isLettersGroup = (charSet.size() == lettersSet.size());
                if (isLettersGroup)
                {
                    for (auto it = lettersSet.begin(); it != lettersSet.end(); ++it)
                    {
                        if (!charSet.contains(*it))
                        {
                            isLettersGroup = false;
                            break;
                        }
                    }
                }
                bool isDigitsGroup = (charSet.size() == digitsSet.size());
                if (isDigitsGroup)
                {
                    for (auto it = digitsSet.begin(); it != digitsSet.end(); ++it)
                    {
                        if (!charSet.contains(*it))
                        {
                            isDigitsGroup = false;
                            break;
                        }
                    }
                }
                if (isLettersGroup)
                    conds << "isalpha(ch)";
                else if (isDigitsGroup)
                    conds << "isdigit(ch)";
                else
                {
                    for (auto it = charSet.begin(); it != charSet.end(); ++it)
                        conds << QString("ch == '%1'").arg(*it);
                }
            }
            code += QString("        if (%1) { state = %2; ch = GetNext(); moved = true; }\n").arg(conds.join(" || ")).arg(to);
        }
        code += "        if (!moved) done = true;\n        break; }\n";
    }

    code += "      default: done = true; break;\n";
    code += "    }\n  }\n";
    // 结束后根据是否处于接受态输出 token 名称，失败则输出 Error
    QStringList acceptConds;
    for (int s : accepts)
        acceptConds << QString("state == %1").arg(s);
    QString tokenName = QString::fromStdString(regex.id.empty() ? regex.rawRegex : regex.id);
    if (!acceptConds.isEmpty())
        code += QString("  if (%1) cout << \"%2\"; else cout << \"Error!\";\n").arg(acceptConds.join(" || ")).arg(tokenName);
    else
        code += "  cout << \"Error!\";\n";
    code += "}\n";

    code += "int main(){\n";
    code += "  string s; getline(cin, s);\n";
    code += "  if (s.size() > 1023) { cerr << \"Input too long\"; return 1; }\n";
    code += "  pos = 0; for (size_t i = 0; i < s.size(); ++i) buffer[i] = s[i]; buffer[s.size()] = '\\0';\n";
    code += QString("  %1();\n").arg(funcName);
    code += "  return 0;\n}";
    return code;
}
