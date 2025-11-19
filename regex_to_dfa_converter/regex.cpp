#include "regex.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <QDebug>
using namespace std;

vector<string> identifiers;

Regex::Regex(string inputContent, string id) : id(id), rawRegex(inputContent) {
    // if (!id.empty()) identifiers.push_back(id);
}

/** 用于展开功能的特殊字符 */
bool isSpecialChar(char c) {
    return c == '*' || c == '+' || c == '?' || c == '|' ||
           c == '(' || c == ')' || c == '[' || c == ']' ||
           c == '\\' || c == '&' || c == '=';
}

/** 是用户定义的标识符名称 */
bool isIdentifier(string identifier) {
    for (string id : identifiers) {
        if (id == identifier) return true;
    }
    return false;
}

/** 将中括号表示的范围转换为完整的选择表达式 */
string expandRange(string& range) {
    string result = "(";
    size_t i = 0;

    while (i < range.length()) {
        if (i + 2 < range.length() && range[i + 1] == '-') {
            // 处理范围，例如 [a-z]
            char start = range[i];
            char end = range[i + 2];
            for (char c = start; c <= end; c++) {
                result += c;
                if (c != end) {
                    result += '|';
                }
            }
            i += 3; // 跳过当前的范围
            if (i < range.length()) result += '|';
        } else {
            // 如果没有范围，直接加入字符
            result += range[i];
            if (i + 1 < range.length()) {
                result += '|';
            }
            i++;
        }
    }
    result += ")";
    return result;
}

/** 将正则中需要连接的地方添加连接符号'&' */
string concatContent(const string &s) {
    // 预处理：按标识符长度降序排列，优先匹配长标识符
    vector<string> sortedIds(identifiers);
    sort(sortedIds.begin(), sortedIds.end(),
         [](const string& a, const string& b) { return a.size() > b.size(); });

    string result;
    size_t i = 0;

    auto getTokenType = [&](size_t pos) -> char {
        // 返回token类型：'C'普通字符，'I'标识符，'('左括号，')'右括号，'|'管道，'*?+'操作符
        for (const auto& id : sortedIds) {
            if (pos + id.size() <= s.size() && s.substr(pos, id.size()) == id)
                return 'I';
        }
        const char c = s[pos];
        if (c == '(') return '(';
        if (c == ')') return ')';
        if (c == '|') return '|';
        if (c == '*' || c == '?' || c == '+') return 'O';
        return 'C';
    };

    while (i < s.size()) {
        // 处理当前token
        char currType = getTokenType(i);
        size_t tokenLen = 1;

        // 处理标识符匹配
        if (currType == 'I') {
            for (const auto& id : sortedIds) {
                if (i + id.size() <= s.size() && s.substr(i, id.size()) == id) {
                    tokenLen = id.size();
                    break;
                }
            }
        }

        // 添加当前token到结果
        result += s.substr(i, tokenLen);
        i += tokenLen;

        // 判断是否需要添加连接符
        if (i < s.size()) {
            const char nextType = getTokenType(i);
            const bool needsConcat =
                (currType == 'C' || currType == 'I' || currType == ')' || currType == 'O') &&
                (nextType == 'C' || nextType == 'I' || nextType == '(');

            if (needsConcat) result += "&";
        }
    }
    return result;
}

/** 对用户输入的正则表达式进行处理
    1. 提取标识符
    2. 去除空格
    3. 通过 & 表示连接
    4. 对[ ]操作进行预处理，即转换为选择 (|) 形式
    5. 对+操作进行预处理，如 a+ -> a&a*
    6. 对?操作进行预处理，如 a? -> (a|#)
    7. 如果有id，则生成相应的函数 bool is_id(char ch)；
*/
void preprocessRegex(Regex &regex) {
    string rawRegex = "";

    // 去除所有空格
    for (size_t i = 0; i < regex.rawRegex.size(); i++) {
        if (regex.rawRegex[i] == ' ') continue;
        rawRegex += regex.rawRegex[i];
    }
    bool isId = true;
    string id = "";
    string result = "";
    for (size_t i = 0; i < rawRegex.size(); i++) {
        if (isSpecialChar(rawRegex[i])) {
            isId = false;
            if (rawRegex[i] == '[') {
                string range = "";
                i++; //不读取'['
                while(i < rawRegex.size() && rawRegex[i] != ']') {
                    // 一直读取普通字符，直到遇到']'
                    range += rawRegex[i];
                    i++;
                }
                result += expandRange(range);
            } else if (rawRegex[i] == '\\') { // 转义符
                i++;
                // result += "\\";
                result += rawRegex[i];
            } else if (rawRegex[i] == '(') {
                while(i < rawRegex.size() && rawRegex[i] != ')') {
                    // 一直读取字符，直到遇到')'
                    result += rawRegex[i];
                    i++;
                }
                result += rawRegex[i]; // 加上')'
            } else if (rawRegex[i] == '+') {
                char pre = rawRegex[i-1];
                bool isNormal = true; // 区分是不是普通的字符
                // 有可能是()或者[]
                if (pre == ')' || pre == ']') {
                    size_t startPos = result.find_last_of("(");
                    size_t len = result.length() - startPos;
                    result += result.substr(startPos, len);
                    isNormal = false;
                }
                // 有可能是用户定义的标识符
                for (string &id : identifiers) {
                    if (int(i) - int(id.length()) >= 0 && rawRegex.substr(i - id.length(), id.length()) == id) {
                        result += rawRegex.substr(i - id.length(), id.length());  // 如果是，直接加入到结果中
                        isNormal = false;
                        break;
                    }
                }
                if (isNormal) {
                    result += pre;
                }
                result += '*';
            } else if (rawRegex[i] == '?') {
                char pre = rawRegex[i-1];
                bool isNormal = true; // 区分是不是普通的字符
                // 有可能是()或者[]
                if (pre == ')' || pre == ']') {
                    size_t startPos = result.find_last_of("(");
                    result.insert(startPos, 1, '(');  // 插入前置'('
                    isNormal = false;
                }
                // 有可能是用户定义的标识符
                for (string &id : identifiers) {
                    if (int(i) - int(id.length()) >= 0 && rawRegex.substr(i - id.length(), id.length()) == id) {
                        size_t startPos = result.size() - id.length();
                        result.insert(startPos, 1, '(');  // 插入前置'('
                        isNormal = false;
                        break;
                    }
                }
                if (isNormal) {
                    result.insert(result.size() - 1, 1, '(');
                }
                result += "|#)";
            } else {
                result += rawRegex[i];
            }
        }  else {
            string temp = "";
            while(i < rawRegex.size() && !isSpecialChar(rawRegex[i])) {
                // 一直读取普通字符，直到再遇到特殊字符
                temp += rawRegex[i];
                i++;
            }
            if (isId && rawRegex[i] == '=') {
                id += temp;
            } else {
                result += temp;
                i--; // 返还多读取的字符
            }
        }
    }
    regex.content = concatContent(result);
    regex.id = id;
    if (id.size() == 0) {
        regex.rawContent = regex.rawRegex;
    } else {
        regex.rawContent = regex.rawRegex.substr(id.length());
    }

    if (!id.empty())  {
        identifiers.push_back(id);
    }
}

int getPriority(char &op) // 返回运算符优先级
{
    switch (op) {
    case '(':
    case ')':
        return 0;
    case '*':
        return 3;
    case '|':
        return 1;
    case '&':
        return 2;
    default:
        return -1;
    }
}

bool isOperator(char &ch)
{
    switch (ch) {
    case '(':
    case ')':
    case '*':
    case '|':
    case '&':
        return true;
    default:
        return false;
    }
}

/** 中序表达式转后序表达式 */
string regexToPostFix(string& regex) {
    stack<char> opStack;
    string result = "";

    for (size_t i = 0; i < regex.length(); i++) {
        char op = regex[i];
        if (!isOperator(op)) {
            if (op == '\\') {
                continue;
            }
            result += op;
        } else {
            if (op == '(') {
                opStack.push(op); // 左括号直接入栈
            } else if (op == ')')  {
                // 弹出栈中所有操作符直到遇到左括号
                while (!opStack.empty() && opStack.top() != '(') {
                    result += opStack.top();
                    opStack.pop();
                }
                if (!opStack.empty()) opStack.pop(); // 弹出左括号
            } else {
                // 运算符处理
                while (!opStack.empty() && getPriority(opStack.top()) >= getPriority(op)) {
                    result += opStack.top();
                    opStack.pop();
                }
                opStack.push(op);
            }
        }
    }

    while (!opStack.empty()) {
        result += opStack.top();
        opStack.pop();
    }

    return result;
}

NFA regexToNFA(string &postfixRegex) {
    stack<NFA> nfaStack;  // 通过栈存储NFA子图
    int stateCounter = 0;

    for (size_t i = 0; i < postfixRegex.size(); i++) {
        char ch = postfixRegex[i];
        if (isOperator(ch)) {
            if (ch == '&') {
                if (nfaStack.size() < 2) continue;
                NFA nfa2 = nfaStack.top(); nfaStack.pop();
                NFA nfa1 = nfaStack.top(); nfaStack.pop();
                // 连接nfa1的结束状态与nfa2的开始状态
                for (int accept : nfa1.acceptStates) {
                    nfa1.transitions.push_back(Transition(accept, nfa2.startState));
                }
                // 合并状态与转移
                nfa1.acceptStates = nfa2.acceptStates;
                for (Transition t : nfa2.transitions) {
                    nfa1.transitions.push_back(t);
                }
                nfa1.stateCounter = max(nfa1.stateCounter, nfa2.stateCounter);

                nfaStack.push(nfa1);
            } else if (ch == '|') {
                if (nfaStack.size() < 2) continue;
                NFA nfa2 = nfaStack.top(); nfaStack.pop();
                NFA nfa1 = nfaStack.top(); nfaStack.pop();
                // 创建新开始状态与结束状态
                int newStart = stateCounter++;
                int newAccept = stateCounter++;

                NFA newNFA;
                newNFA.startState = newStart;
                newNFA.acceptStates.insert(newAccept);
                newNFA.stateCounter = max(nfa1.stateCounter, nfa2.stateCounter) + 2;
                // 连接新开始状态到两个nfa的开始状态
                newNFA.transitions.push_back(Transition(newStart, nfa1.startState));
                newNFA.transitions.push_back(Transition(newStart, nfa2.startState));
                // 连接两个nfa的结束状态到新结束状态
                for (int accept : nfa1.acceptStates) {
                    newNFA.transitions.push_back(Transition(accept, newAccept));
                }
                for (int accept : nfa2.acceptStates) {
                    newNFA.transitions.push_back(Transition(accept, newAccept));
                }
                // 合并转移
                for (Transition t : nfa1.transitions) {
                    newNFA.transitions.push_back(t);
                }
                for (Transition t : nfa2.transitions) {
                    newNFA.transitions.push_back(t);
                }

                nfaStack.push(newNFA);
            } else if (ch == '*') {
                if (nfaStack.empty()) continue;
                NFA nfa = nfaStack.top(); nfaStack.pop();
                // 创建新开始状态与结束状态
                int newStart = stateCounter++;
                int newAccept = stateCounter++;

                NFA newNFA;
                newNFA.startState = newStart;
                newNFA.acceptStates.insert(newAccept);
                newNFA.stateCounter = nfa.stateCounter + 2;
                // 创建从新开始状态到新结束状态的epsilon转移
                newNFA.transitions.push_back(Transition(newStart, newAccept));

                newNFA.transitions.push_back(Transition(newStart, nfa.startState));
                for (int accept : nfa.acceptStates) {
                    newNFA.transitions.push_back(Transition(accept, newAccept)); // 指向结束状态
                    newNFA.transitions.push_back(Transition(accept, nfa.startState)); // 指向开始状态
                }

                // 合并转移
                for (Transition t : nfa.transitions) {
                    newNFA.transitions.push_back(t);
                }

                nfaStack.push(newNFA);
            }
        } else {
            // 不是操作符，创建基本NFA: (start) --ch--> ((end))
            string by = "";
            bool isId = false;
            for (string id : identifiers) {
                if (id.find(ch) != string::npos && postfixRegex.substr(i, id.size()) == id) {
                    by += id; // 是用户定义的标识符
                    isId = true;
                    i += id.size() - 1; // 循环结束后还会再+1
                }
            }
            if (!isId) {
                by += ch;
            }

            int newStart = stateCounter++;
            int newAccept = stateCounter++;

            NFA basicNFA;
            basicNFA.startState = newStart;
            basicNFA.acceptStates.insert(newAccept);
            basicNFA.transitions.push_back(Transition(newStart, newAccept, by));
            nfaStack.push(basicNFA);
        }
    }

    return nfaStack.empty() ? NFA() : nfaStack.top();
}

void initNFA(Regex &regex) {
    if (regex.content.empty()) {
        preprocessRegex(regex);
    }
    string postfix = regexToPostFix(regex.content);
    regex.nfa = regexToNFA(postfix);
}

/** 获取状态的epsilon闭包 */
set<int> getEpsilonClosure(NFA &nfa, set<int> &states) {
    set<int> result = states;
    stack<int> stack;

    for(int state : states) {
        stack.push(state);
    }

    // DFS
    while (!stack.empty()) {
        int curr = stack.top();
        stack.pop();

        for (Transition &t : nfa.transitions) {
            if (t.from == curr && t.symbol == "#") {
                if(result.find(t.to) == result.end()) {
                    // 找到此状态出发的空状态，并且该状态不在result中
                    result.insert(t.to);
                    stack.push(t.to);
                }
            }
        }
    }
    return result;
}

// 获取状态集合通过某个符号的转移
set<int> getMove(NFA &nfa, set<int> &states, string symbol) {
    set<int> result;

    for (int state : states) {
        for (Transition &t : nfa.transitions) {
            if (t.from == state && t.symbol == symbol) {
                result.insert(t.to);
            }
        }
    }

    return result;
}

DFA nfaToDFA(NFA &nfa) {
    DFA dfa;
    map<set<int>, int> dfaStates; // DFA的状态集（是NFA状态集的子集）
    queue<set<int> > queue; // 存放待处理的NFA状态集

    set<int> nfaStartState = { nfa.startState };
    set<int> startStateSet = getEpsilonClosure(nfa, nfaStartState);
    dfaStates[startStateSet] = 0;
    dfa.startState = startStateSet;
    dfa.stateCounter = 0;
    queue.push(startStateSet);

    set<string> symbols; // 输入符号集合
    for (Transition &t : nfa.transitions) {
        if (t.symbol != "#") {
            symbols.insert(t.symbol);
        }
    }

    // 子集构造法
    while (!queue.empty()) {
        set<int> currentStateSet = queue.front();
        queue.pop();

        for (int state : currentStateSet) {
            if (nfa.acceptStates.find(state) != nfa.acceptStates.end()) {
                dfa.acceptStates.insert(currentStateSet);
                break;
            }
        }

        for (string symbol : symbols) {
            set<int> moveToStateSet = getMove(nfa, currentStateSet, symbol);
            set<int> nextStateSet = getEpsilonClosure(nfa, moveToStateSet);

            if (nextStateSet.empty()) continue;

            if (dfaStates.find(nextStateSet) == dfaStates.end()) {
                dfaStates[nextStateSet] = dfa.stateCounter++;
                queue.push(nextStateSet);
            }
            // 添加DFA转移
            dfa.transitions.push_back(DFATransition(currentStateSet, nextStateSet, symbol));
        }
    }

    return dfa;
}

void initDFA(Regex &regex) {
    if (regex.nfa.transitions.empty()) {
        initNFA(regex);
    }
    NFA nfa = regex.nfa;
    regex.dfa = nfaToDFA(nfa);
}

// 最小化DFA Hopcroft 算法
DFA dfaToMinDFA(DFA &dfa) {
    DFA minDFA;
    if (dfa.transitions.empty()) {
        return minDFA;
    }

    set<set<int>> states;
    set<string> symbolSet;
    for (const DFATransition &t : dfa.transitions) {
        states.insert(t.from);
        states.insert(t.to);
        symbolSet.insert(t.symbol);
    }

    // 初始分区：接受状态和非接受状态
    map<set<int>, int> stateToGroup;  // 状态到分组的映射
    vector<set<set<int>>> groups;     // 分组列表

    set<set<int>> acceptingStates;
    set<set<int>> nonAcceptingStates;

    for (const auto &state : states) {
        if (dfa.acceptStates.find(state) != dfa.acceptStates.end()) {
            acceptingStates.insert(state);
        } else {
            nonAcceptingStates.insert(state);
        }
    }

    if (!acceptingStates.empty()) groups.push_back(acceptingStates);
    if (!nonAcceptingStates.empty()) groups.push_back(nonAcceptingStates);

    // 更新状态到分组的映射
    for (size_t i = 0; i < groups.size(); i++) {
        for (const auto &state : groups[i]) {
            stateToGroup[state] = i;
        }
    }

    // 辅助函数：查找状态在某个符号下的下一个状态
    auto getNextState = [&dfa](const set<int> &state, const string &symbol) -> set<int> {
        for (const DFATransition &t : dfa.transitions) {
            if (t.from == state && t.symbol == symbol) {
                return t.to;
            }
        }
        return set<int>();
    };

    // 分割算法
    bool changed = true;
    while (changed) {
        changed = false;

        vector<set<set<int>>> newGroups;

        for (const auto &group : groups) {
            if (group.size() <= 1) {
                // 如果分组只有一个状态，不需要进一步分割
                newGroups.push_back(group);
                continue;
            }

            map<vector<int>, set<set<int>>> nextGroupMap;

            for (const auto &state : group) {
                // 计算状态的特征向量（在各个符号下转移到哪个分组）
                vector<int> signature;
                for (const auto &symbol : symbolSet) {
                    set<int> nextState = getNextState(state, symbol);

                    if (nextState.empty()) {
                        signature.push_back(-1);  // 没有转移
                    } else {
                        signature.push_back(stateToGroup[nextState]);
                    }
                }

                nextGroupMap[signature].insert(state);
            }

            if (nextGroupMap.size() > 1) {
                changed = true;

                for (const auto &entry : nextGroupMap) {
                    newGroups.push_back(entry.second);
                }
            } else {
                newGroups.push_back(group);
            }
        }

        if (changed) {
            groups = newGroups;
            stateToGroup.clear();
            for (size_t i = 0; i < groups.size(); i++) {
                for (const auto &state : groups[i]) {
                    stateToGroup[state] = i;
                }
            }
        }
    }

    // 构建最小DFA
    map<int, set<int>> groupToState;
    for (size_t i = 0; i < groups.size(); i++) {
        set<int> newState;
        newState.insert(i);
        groupToState[i] = newState;
    }

    // 设置开始状态
    for (size_t i = 0; i < groups.size(); i++) {
        for (const auto &state : groups[i]) {
            if (state == dfa.startState) {
                minDFA.startState = groupToState[i];
                break;
            }
        }
    }

    // 设置接受状态
    for (size_t i = 0; i < groups.size(); i++) {
        for (const auto &state : groups[i]) {
            if (dfa.acceptStates.find(state) != dfa.acceptStates.end()) {
                minDFA.acceptStates.insert(groupToState[i]);
                break;
            }
        }
    }

    // 创建转移
    for (const DFATransition &t : dfa.transitions) {
        int fromGroup = stateToGroup[t.from];
        int toGroup = stateToGroup[t.to];

        // 检查是否已经存在相同的转移
        bool exists = false;
        for (const DFATransition &existing : minDFA.transitions) {
            if (existing.from == groupToState[fromGroup] &&
                existing.to == groupToState[toGroup] &&
                existing.symbol == t.symbol) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            minDFA.transitions.push_back(DFATransition(
                groupToState[fromGroup],
                groupToState[toGroup],
                t.symbol
                ));
        }
    }

    minDFA.stateCounter = groups.size();
    return minDFA;
}

void initMinDFA(Regex &regex) {
    if (regex.dfa.transitions.empty()) {
        initDFA(regex);
    }
    DFA dfa = regex.dfa;
    regex.minDfa = dfaToMinDFA(dfa);
}
