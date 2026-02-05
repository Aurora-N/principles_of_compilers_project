#include "grammar.h"
#include <algorithm>
#include <queue>

namespace { // 匿名命名空间，用于辅助函数
string formatProductionWithDot(const Production& prod, int dotPosition, const string& epsilonSymbol) {
    string str = prod.lhs + " ->";
    bool dotPlaced = false;
    if (prod.rhs.empty() && dotPosition == 0) {
        str += " .";
        dotPlaced = true;
    } else if (prod.rhs.size() == 1 && prod.rhs[0] == epsilonSymbol) {
        if (dotPosition == 0) {
            str += " . " + epsilonSymbol;
            dotPlaced = true;
        } else if (dotPosition == 1) {
            str += " " + epsilonSymbol + " .";
            dotPlaced = true;
        }
    }

    if (!dotPlaced) {
        for (int i = 0; i < static_cast<int>(prod.rhs.size()); ++i) {
            if (i == dotPosition) {
                str += " .";
            }
            str += " " + prod.rhs[i];
        }
        if (dotPosition == static_cast<int>(prod.rhs.size())) {
            str += " .";
        }
    }
    return str;
}
} // 结束匿名命名空间

const string Grammar::EPSILON = "#";
const string Grammar::END_OF_INPUT = "$";
const string Grammar::AUGMENTED_START_SYMBOL = "S'";

// 辅助函数：获取LR0Item的字符串表示
string LR0Item::toString(const vector<Production>& allProductions) const {
    if (productionIndex < 0 || static_cast<size_t>(productionIndex) >= allProductions.size()) {
        return "[无效的产生式索引]";
    }
    const auto& prod = allProductions[productionIndex];
    return formatProductionWithDot(prod, dotPosition, Grammar::EPSILON);
}

// 辅助函数：获取LR1Item的字符串表示
string LR1Item::toString(const vector<Production>& allProductions) const {
    if (productionIndex < 0 || static_cast<size_t>(productionIndex) >= allProductions.size()) {
        return "[无效的产生式索引]";
    }
    const auto& prod = allProductions[productionIndex];
    string itemStr = formatProductionWithDot(prod, dotPosition, Grammar::EPSILON);
    return "[" + itemStr + ", " + lookahead + "]";
}

string ParsingAction::toString(const vector<Production>& allProductions) const {
    switch (type) {
        case Type::SHIFT:
            return "S" + to_string(value);
        case Type::REDUCE: {
            if (value < 0 || static_cast<size_t>(value) >= allProductions.size()) {
                return "R_错误(索引)";
            }
            // 'value' 是产生式索引。R_k 使用基于1的原始规则编号。
            if (allProductions[value].isAugmented) {
                 return "R_增广(" + to_string(value) + ")";
            }
            if (allProductions[value].originalRuleIndex > 0) { // 如果设置了originalRuleIndex，则使用基于1的originalRuleIndex。
                 return "R" + to_string(allProductions[value].originalRuleIndex);
            }
            return "R?(" + to_string(value) + ")";
        }
        case Type::ACCEPT:
            return "Acc";
        case Type::ERROR:
            return ""; // 错误时返回空白
        case Type::CONFLICT:
            return "冲突:" + conflictDetails.substr(0,4);
        default:
            return "??";
    }
}

vector<Grammar::ParsingStep> Grammar::analyzeSourceCode(const string& sourceCode) {
    // 清除之前的结果
    if (syntaxTreeRoot) {
        delete syntaxTreeRoot;
        syntaxTreeRoot = nullptr;
    }
    syntaxErrors.clear();

    vector<ParsingStep> steps;

    // 检查LR(1)分析表是否可用
    if (lr1ActionTable_.empty() || lr1GotoTable_.empty()) {
        syntaxErrors.push_back("错误: LR(1)分析表不可用，请先解析文法。");
        return steps;
    }

    // 辅助函数：判断是否为数字（整数或浮点数）
    auto isNumber = [](const string& token) -> bool {
        if (token.empty()) return false;
        bool hasDecimalPoint = false;
        size_t start = 0;
        if (token[0] == '-' || token[0] == '+') {
            if (token.length() == 1) return false;
            start = 1;
        }
        for (size_t i = start; i < token.length(); ++i) {
            if (token[i] == '.') {
                if (hasDecimalPoint) return false; // 不允许多个小数点
                hasDecimalPoint = true;
            } else if (!isdigit(static_cast<unsigned char>(token[i]))) {
                return false;
            }
        }
        // 至少要有一个数字
        return (token.length() > start) && (token[start] != '.' || token.length() > start + 1);
    };

    // 辅助函数：判断是否为标识符（字母或下划线开头，后跟字母、数字或下划线）
    auto isIdentifier = [](const string& token) -> bool {
        if (token.empty()) return false;
        char first = token[0];
        if (!isalpha(static_cast<unsigned char>(first)) && first != '_') return false;
        for (size_t i = 1; i < token.length(); ++i) {
            char c = token[i];
            if (!isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
        }
        return true;
    };

    // 词法分析：将源代码分割为token
    vector<string> inputTokens;      // 用于分析的token（可能被映射为NUM/ID）
    vector<string> originalTokens;   // 原始token值（用于语法树显示）
    
    string currentToken;
    for (size_t i = 0; i < sourceCode.length(); ++i) {
        char c = sourceCode[i];
        if (isspace(c)) {
            if (!currentToken.empty()) {
                // 进行token分类
                string mappedToken = currentToken;
                if (terminals.count("NUM") && isNumber(currentToken)) {
                    mappedToken = "NUM";
                } else if (terminals.count("ID") && isIdentifier(currentToken)) {
                    // 检查是否为保留字/关键字（已在终结符中定义的符号）
                    if (!terminals.count(currentToken) && !nonTerminals.count(currentToken)) {
                        mappedToken = "ID";
                    }
                }
                inputTokens.push_back(mappedToken);
                originalTokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }
    if (!currentToken.empty()) {
        // 进行token分类
        string mappedToken = currentToken;
        if (terminals.count("NUM") && isNumber(currentToken)) {
            mappedToken = "NUM";
        } else if (terminals.count("ID") && isIdentifier(currentToken)) {
            // 检查是否为保留字/关键字
            if (!terminals.count(currentToken) && !nonTerminals.count(currentToken)) {
                mappedToken = "ID";
            }
        }
        inputTokens.push_back(mappedToken);
        originalTokens.push_back(currentToken);
    }
    inputTokens.push_back(END_OF_INPUT); // 添加结束符号 $
    originalTokens.push_back(END_OF_INPUT);

    // 初始化状态栈和符号栈
    vector<int> stateStack;
    vector<string> symbolStack;
    vector<SyntaxTreeNode*> nodeStack; // 语法树节点栈

    stateStack.push_back(0); // 初始状态
    symbolStack.push_back("$"); // 栈底符号

    size_t inputPos = 0;
    int stepId = 1;
    bool accepted = false;
    bool errorOccurred = false;

    // 记录初始步骤
    ParsingStep initialStep;
    initialStep.stepId = stepId++;
    initialStep.stateStack = stateStack;
    initialStep.symbolStack = symbolStack;
    for (size_t i = inputPos; i < inputTokens.size(); ++i) {
        // 显示时包含原始值
        if (inputTokens[i] != originalTokens[i]) {
            initialStep.remainingInput.push_back(inputTokens[i] + "(" + originalTokens[i] + ")");
        } else {
            initialStep.remainingInput.push_back(inputTokens[i]);
        }
    }
    initialStep.action = "Initial";
    initialStep.details = "初始化";
    steps.push_back(initialStep);

    while (!accepted && !errorOccurred) {
        int currentState = stateStack.back();
        string currentInput = (inputPos < inputTokens.size()) ? inputTokens[inputPos] : END_OF_INPUT;
        string currentOriginal = (inputPos < originalTokens.size()) ? originalTokens[inputPos] : END_OF_INPUT;

        // 查找ACTION表
        ParsingAction action;
        if (currentState < static_cast<int>(lr1ActionTable_.size()) &&
            lr1ActionTable_[currentState].count(currentInput)) {
            action = lr1ActionTable_[currentState].at(currentInput);
        } else {
            action.type = ParsingAction::Type::ERROR;
        }

        ParsingStep step;
        step.stepId = stepId++;
        step.stateStack = stateStack;
        step.symbolStack = symbolStack;
        for (size_t i = inputPos; i < inputTokens.size(); ++i) {
            // 显示时包含原始值
            if (inputTokens[i] != originalTokens[i]) {
                step.remainingInput.push_back(inputTokens[i] + "(" + originalTokens[i] + ")");
            } else {
                step.remainingInput.push_back(inputTokens[i]);
            }
        }

        switch (action.type) {
            case ParsingAction::Type::SHIFT: {
                // 移进操作
                int nextState = action.value;
                stateStack.push_back(nextState);
                symbolStack.push_back(currentInput);

                // 为终结符创建叶节点，保存原始值
                SyntaxTreeNode* leafNode = new SyntaxTreeNode(currentInput, currentOriginal);
                nodeStack.push_back(leafNode);

                step.action = "Shift";
                string displayToken = (currentInput != currentOriginal) ? 
                    currentInput + "(" + currentOriginal + ")" : currentInput;
                step.details = "移进 " + displayToken + "，转到状态 " + to_string(nextState);
                inputPos++;
                break;
            }
            case ParsingAction::Type::REDUCE: {
                // 归约操作
                int prodIndex = action.value;
                const Production& prod = productions[prodIndex];
                int rhsLength = static_cast<int>(prod.rhs.size());

                // 处理空产生式 (epsilon)
                if (rhsLength == 1 && prod.rhs[0] == EPSILON) {
                    rhsLength = 0;
                }

                // 从栈中弹出rhsLength个元素，收集子节点
                vector<SyntaxTreeNode*> childNodes;
                for (int j = 0; j < rhsLength; ++j) {
                    stateStack.pop_back();
                    symbolStack.pop_back();
                    if (!nodeStack.empty()) {
                        childNodes.push_back(nodeStack.back());
                        nodeStack.pop_back();
                    }
                }

                // 子节点需要反转（因为是从栈顶弹出的，顺序是反的）
                std::reverse(childNodes.begin(), childNodes.end());

                // 创建语法树节点，根据产生式类型进行优化处理
                SyntaxTreeNode* newNode = nullptr;

                // 检测括号产生式模式: X -> ( Y ) 
                // 括号不进入语法树，只保留中间的表达式
                if (rhsLength == 3 && 
                    prod.rhs[0] == "(" && prod.rhs[2] == ")" &&
                    childNodes.size() == 3) {
                    // 删除左括号和右括号节点，只保留中间节点
                    delete childNodes[0]; // 删除 '('
                    delete childNodes[2]; // 删除 ')'
                    newNode = childNodes[1]; // 直接使用中间的节点
                }
                // 检测二元操作符模式: X -> Y op Z (如 E -> E + T)
                // 让操作符成为父节点
                else if (rhsLength == 3 && 
                         childNodes.size() == 3 &&
                         nonTerminals.count(prod.rhs[0]) && 
                         nonTerminals.count(prod.rhs[2]) &&
                         terminals.count(prod.rhs[1])) {
                    // 操作符作为父节点
                    SyntaxTreeNode* opNode = childNodes[1]; // 操作符节点
                    opNode->children.push_back(childNodes[0]); // 左操作数
                    opNode->children.push_back(childNodes[2]); // 右操作数
                    newNode = opNode;
                }
                // 检测一元操作符模式: X -> op Y (如 F -> - F)
                else if (rhsLength == 2 &&
                         childNodes.size() == 2 &&
                         terminals.count(prod.rhs[0]) &&
                         nonTerminals.count(prod.rhs[1])) {
                    // 一元操作符作为父节点
                    SyntaxTreeNode* opNode = childNodes[0]; // 操作符节点
                    opNode->children.push_back(childNodes[1]); // 操作数
                    newNode = opNode;
                }
                // 检测简单传递产生式: X -> Y (如 E -> T, T -> F)
                // 直接传递子节点，避免不必要的中间层
                else if (rhsLength == 1 && 
                         childNodes.size() == 1 &&
                         nonTerminals.count(prod.rhs[0])) {
                    // 直接传递子节点，不创建新的中间节点
                    newNode = childNodes[0];
                }
                // 默认情况：创建以非终结符为父节点的子树
                else {
                    newNode = new SyntaxTreeNode(prod.lhs);
                    for (SyntaxTreeNode* child : childNodes) {
                        newNode->children.push_back(child);
                    }
                }

                // 将新非终结符压栈
                symbolStack.push_back(prod.lhs);
                nodeStack.push_back(newNode);

                // 查找GOTO表获取新状态
                int gotoState = -1;
                int topState = stateStack.back();
                if (topState < static_cast<int>(lr1GotoTable_.size()) &&
                    lr1GotoTable_[topState].count(prod.lhs)) {
                    gotoState = lr1GotoTable_[topState].at(prod.lhs);
                }

                if (gotoState >= 0) {
                    stateStack.push_back(gotoState);
                    step.action = "Reduce";
                    string prodStr = prod.lhs + " ->";
                    for (const string& s : prod.rhs) {
                        prodStr += " " + s;
                    }
                    step.details = "用产生式 " + prodStr + " 归约，转到状态 " + to_string(gotoState);
                } else {
                    step.action = "Error";
                    step.details = "GOTO表中找不到 (" + to_string(topState) + ", " + prod.lhs + ")";
                    syntaxErrors.push_back(step.details);
                    errorOccurred = true;
                }
                break;
            }
            case ParsingAction::Type::ACCEPT: {
                step.action = "Accept";
                step.details = "接受！语法分析成功。";
                accepted = true;

                // 设置语法树根节点
                if (!nodeStack.empty()) {
                    syntaxTreeRoot = nodeStack.back();
                    nodeStack.pop_back();
                }
                break;
            }
            case ParsingAction::Type::CONFLICT: {
                step.action = "Error";
                step.details = "分析表存在冲突: " + action.conflictDetails;
                syntaxErrors.push_back("状态 " + to_string(currentState) + ", 输入 '" + currentInput + "': " + action.conflictDetails);
                errorOccurred = true;
                break;
            }
            case ParsingAction::Type::ERROR:
            default: {
                step.action = "Error";
                string displayToken = (currentInput != currentOriginal) ? 
                    currentInput + "(" + currentOriginal + ")" : currentInput;
                step.details = "语法错误: 在状态 " + to_string(currentState) + " 遇到意外符号 '" + displayToken + "'";
                syntaxErrors.push_back(step.details);
                errorOccurred = true;
                break;
            }
        }

        steps.push_back(step);
    }

    // 清理未使用的节点（如果发生错误）
    if (errorOccurred) {
        for (SyntaxTreeNode* node : nodeStack) {
            delete node;
        }
        nodeStack.clear();
    }

    return steps;
}

Grammar::Grammar() {
    clear();
}

void Grammar::clear() {
    startSymbol.clear();
    originalStartSymbol.clear();
    augmentedStartSymbol.clear();
    nonTerminals.clear();
    terminals.clear();
    productions.clear();
    firstSets.clear();
    followSets.clear();
    errorMessage.clear();

    lr0States.clear();
    lr0Transitions.clear();
    slrConflictMessages_.clear();
    isSLR1ConflictFree_ = true;

    lr1States_.clear();
    lr1Transitions_.clear();
    lr1ActionTable_.clear();
    lr1GotoTable_.clear();
    isLR1TableConflictFree_ = true;
    lr1TableConflictMessages_.clear();

    if (syntaxTreeRoot) {
        delete syntaxTreeRoot;
        syntaxTreeRoot = nullptr;
    }
    syntaxErrors.clear();
}

bool Grammar::parseGrammar(const QString& grammarInput) {
    clear();
    QStringList lines = grammarInput.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        errorMessage = "文法输入为空。";
        return false;
    }

    if (!identifyNonTerminalsAndStartSymbol(lines)) { // 阶段1：识别所有非终结符和开始符号
        return false;
    }
    originalStartSymbol = this->startSymbol; // 存储解析出的开始符号

    if (!parseProductionsAndTerminals(lines)) { // 阶段2：解析产生式和终结符
        return false;
    }

    augmentGrammar(); // 进行文法扩充
    if (!errorMessage.empty()) {
        return false;
    }
    if (productions.empty() || productions[0].lhs != this->augmentedStartSymbol) {
        if (errorMessage.empty()) errorMessage = "文法增广失败或未在开头产生预期的S'规则。";
        return false;
    }

    calculateFirstSets();
    calculateFollowSets();
    buildLR0DFA();
    checkForSLR1Conflicts();
    buildLR1DFA();
    buildLR1ParsingTable();

    return true;
}

// 阶段1：从规则的LHS识别所有非终结符，并确定开始符号。
bool Grammar::identifyNonTerminalsAndStartSymbol(const QStringList& lines) {
    bool firstNonTerminalFound = false;
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;

        QStringList parts = trimmedLine.split("->");
        if (parts.size() != 2) {
            errorMessage = "无效的规则格式 (缺少或过多的 '->'): " + trimmedLine.toStdString();
            return false;
        }

        string lhs = parts[0].trimmed().toStdString();
        if (lhs.empty()) {
            errorMessage = "规则中的LHS为空: " + trimmedLine.toStdString();
            return false;
        }
        // 基本LHS验证
        if (lhs.find(' ') != string::npos || lhs.find('\t') != string::npos) {
            errorMessage = "LHS非终结符包含空格: '" + lhs + "' 在规则: " + trimmedLine.toStdString();
            return false;
        }
        if (lhs == EPSILON) {
            errorMessage = "LHS不能是Epsilon ('#'): " + trimmedLine.toStdString();
            return false;
        }

        nonTerminals.insert(lhs);
        if (!firstNonTerminalFound) {
            startSymbol = lhs;
            firstNonTerminalFound = true;
        }
    }

    if (nonTerminals.empty()) {
        errorMessage = "未找到非终结符。文法可能为空或格式不正确。";
        return false;
    }
    if (startSymbol.empty()) {
        errorMessage = "无法确定开始符号 (可能未找到产生式?)。";
        return false;
    }
    return true;
}

// 阶段2：解析每个产生式规则并识别终结符。
bool Grammar::parseProductionsAndTerminals(const QStringList& lines) {
    int ruleIndex = 0;
    for (const QString& line : lines) {
        ruleIndex++;
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;

        QStringList parts = trimmedLine.split("->");
        if (parts.size() != 2) { return false; }

        string lhs = parts[0].trimmed().toStdString();
        QStringList rhsAlternatives = parts[1].split('|');

        if (rhsAlternatives.isEmpty()) {
            errorMessage = "规则RHS为空: " + lhs;
            return false;
        }

        for (const QString& alt : rhsAlternatives) {
            string rhsString = alt.trimmed().toStdString();
            if (rhsString.empty()) {
                errorMessage = "规则RHS中的选项为空: " + lhs;
                return false;
            }

            vector<string> rhsSymbols = tokenizeRHS(rhsString, ruleIndex);
            if (rhsSymbols.empty() && rhsString != EPSILON) {
                 if (errorMessage.empty()) errorMessage = "RHS符号化失败 '" + rhsString + "' 在规则 " + to_string(ruleIndex);
                 return false;
            }
            
            Production rule = {lhs, {}, ruleIndex};
            if (rhsString == EPSILON) {
                rule.rhs.push_back(EPSILON);
            } else {
                for (const string& symbol : rhsSymbols) {
                    if (symbol.empty()) {
                        errorMessage = "在规则 '" + lhs + " -> " + rhsString + "' (规则行 " + to_string(ruleIndex) + ") 的RHS中发现空符号。";
                        return false;
                    }
                    rule.rhs.push_back(symbol);
                    if (nonTerminals.find(symbol) == nonTerminals.end() && symbol != EPSILON) {
                        terminals.insert(symbol);
                    }
                }
            }
            productions.push_back(rule);
        }
    }
    return true;
}

// 将产生式规则的右侧 (RHS) 符号化为符号向量。
vector<string> Grammar::tokenizeRHS(const string& rhsString, int ruleIndexForError) {
    vector<string> symbols;
    if (rhsString == EPSILON) {
        symbols.push_back(EPSILON);
        return symbols;
    }

    string currentSymbol;
    for (char c : rhsString) {
        if (isspace(c)) {
            if (!currentSymbol.empty()) {
                symbols.push_back(currentSymbol);
                currentSymbol.clear();
            }
        } else {
            currentSymbol += c;
        }
    }
    if (!currentSymbol.empty()) {
        symbols.push_back(currentSymbol);
    }

    for(const auto& sym : symbols) {
        if (sym == EPSILON && symbols.size() > 1) {
            errorMessage = "Epsilon ('@') 不能作为RHS中符号序列的一部分。规则行: " + to_string(ruleIndexForError);
            return {};
        }
    }

    return symbols;
}

void Grammar::calculateFirstSets() {
    for (const string& nt : nonTerminals) {
        firstSets[nt] = {};
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : productions) {
            const string& lhs = rule.lhs;
            const auto& rhs = rule.rhs;

            if (rhs.empty()) continue;

            set<string> rhsFirst = calculateFirstForSequence(rhs);
            
            size_t oldSize = firstSets[lhs].size();
            for (const string& symbol : rhsFirst) {
                firstSets[lhs].insert(symbol);
            }
            if (firstSets[lhs].size() > oldSize) {
                changed = true;
            }
        }
    }
}

set<string> Grammar::calculateFirstForSequence(const vector<string>& sequence) const {
    set<string> first;
    if (sequence.empty() || (sequence.size() == 1 && sequence[0] == EPSILON) ) {
        first.insert(EPSILON);
        return first;
    }

    // Check for nullable sequence correctly
    bool allDeriveEpsilon = true;
    for (const string& symbol : sequence) {
        if (symbol == EPSILON) {
            // Epsilon itself derives epsilon, continue to next
            continue;
        }

        bool derivesEpsilon = false;
        if (nonTerminals.count(symbol)) {
            const auto& ntFirst = firstSets.at(symbol);
            for (const string& s : ntFirst) {
                if (s == EPSILON) {
                    derivesEpsilon = true;
                } else {
                    first.insert(s);
                }
            }
        } else {
            // Terminal
            first.insert(symbol);
            derivesEpsilon = false;
        }

        if (!derivesEpsilon) {
            allDeriveEpsilon = false;
            break; 
        }
    }

    if (allDeriveEpsilon) {
        first.insert(EPSILON);
    }
    return first;
}

void Grammar::calculateFollowSets() {
    for (const string& nt : nonTerminals) {
        followSets[nt] = {};
    }

    // 使用扩充文法开始符号S'初始化Follow集。
    if (this->startSymbol.empty() || nonTerminals.find(this->startSymbol) == nonTerminals.end()) {
         if (errorMessage.empty()) {
            errorMessage = "无法计算Follow集: 增广开始符号 ('" + this->startSymbol + "') 未定义或不在非终结符中。原始开始符号: '" + originalStartSymbol + "'";
         }
         return;
    }

    followSets[this->startSymbol].insert(END_OF_INPUT);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& rule : productions) {
            const string& A = rule.lhs; // A -> alpha B beta
            const auto& rhs = rule.rhs;

            for (size_t i = 0; i < rhs.size(); ++i) {
                const string& B = rhs[i];
                if (nonTerminals.count(B)) { // B 是非终结符
                    // 情况1：A -> alpha B beta
                    // 创建beta序列
                    vector<string> betaSequence;
                    if (i + 1 < rhs.size()) {
                        for (size_t j = i + 1; j < rhs.size(); ++j) {
                            betaSequence.push_back(rhs[j]);
                        }
                    }

                    set<string> firstOfBeta;
                    if (!betaSequence.empty()) {
                        firstOfBeta = calculateFirstForSequence(betaSequence);
                    }
                    // 否则：A -> alpha B (beta为空)，由下面的情况2处理。

                    size_t oldFollowBSize = followSets[B].size();

                    bool betaHasEpsilon = false;
                    for (const string& term : firstOfBeta) {
                        if (term == EPSILON) {
                            betaHasEpsilon = true;
                        } else {
                            followSets[B].insert(term);
                        }
                    }

                    // 情况2：A -> alpha B (beta为空) 或 A -> alpha B beta 其中 First(beta) 包含 EPSILON
                    if (betaSequence.empty() || betaHasEpsilon) {
                        const auto& followA = followSets.at(A);
                        for (const string& term : followA) {
                            followSets[B].insert(term);
                        }
                    }
                    
                    if (followSets[B].size() > oldFollowBSize) {
                        changed = true;
                    }
                }
            }
        }
    }
}

void Grammar::augmentGrammar() {
    if (originalStartSymbol.empty()) {
        errorMessage = "无法增广文法: 未识别原始开始符号。";
        return;
    }

    augmentedStartSymbol = AUGMENTED_START_SYMBOL;
    // 假设来自AUGMENTED_START_SYMBOL的S'是唯一的，并且不在用户的文法中。

    nonTerminals.insert(augmentedStartSymbol);
    
    Production augRule;
    augRule.lhs = augmentedStartSymbol;
    augRule.rhs = {originalStartSymbol};
    augRule.originalRuleIndex = -1; // 标记为增广规则。
    augRule.isAugmented = true;

    productions.insert(productions.begin(), augRule); // 将 S' -> S 添加到开头。
    
    startSymbol = augmentedStartSymbol; // 更新内部开始符号以用于解析算法。
}

const map<string, set<string>>& Grammar::getFirstSets() const {
    return firstSets;
}

const map<string, set<string>>& Grammar::getFollowSets() const {
    return followSets;
}

const vector<Production>& Grammar::getProductions() const {
    return productions;
}

const set<string>& Grammar::getNonTerminals() const {
    return nonTerminals;
}

const set<string>& Grammar::getTerminals() const {
    return terminals;
}

const string& Grammar::getStartSymbol() const {
    // 返回原始开始符号以供用户显示。
    return originalStartSymbol;
}

string Grammar::getErrorMessage() const {
    return errorMessage;
}

// LR(0) DFA 构建
LR0State Grammar::calculateClosure(const LR0State& items) const {
    LR0State closureSet = items;
    queue<LR0Item> worklist;

    for (const auto& item : items) {
        worklist.push(item);
    }

    while (!worklist.empty()) {
        LR0Item currentItem = worklist.front();
        worklist.pop();

        const Production& prod = productions[currentItem.productionIndex];

        if (currentItem.dotPosition < static_cast<int>(prod.rhs.size())) {
            const string& symbolAfterDot = prod.rhs[currentItem.dotPosition];
            if (nonTerminals.count(symbolAfterDot)) {
                // 对于每个产生式 B -> gamma，添加 [B -> .gamma]
                for (size_t i = 0; i < productions.size(); ++i) {
                    if (productions[i].lhs == symbolAfterDot) {
                        LR0Item newItem(static_cast<int>(i), 0);
                        if (closureSet.find(newItem) == closureSet.end()) {
                            closureSet.insert(newItem);
                            worklist.push(newItem);
                        }
                    }
                }
            }
        }
    }
    return closureSet;
}

LR0State Grammar::calculateGoto(const LR0State& items, const string& symbol) const {
    LR0State gotoSet;
    for (const auto& item : items) {
        const Production& prod = productions[item.productionIndex];
        if (item.dotPosition < static_cast<int>(prod.rhs.size())) {
            if (prod.rhs[item.dotPosition] == symbol) {
                gotoSet.insert(LR0Item(item.productionIndex, item.dotPosition + 1));
            }
        }
    }
    return calculateClosure(gotoSet);
}

void Grammar::buildLR0DFA() {
    lr0States.clear();
    lr0Transitions.clear();

    if (productions.empty() || productions[0].lhs != augmentedStartSymbol || productions[0].rhs.empty()) {
        errorMessage = "无法构建LR(0) DFA: 文法未正确增广或为空。";
        return;
    }
    
    // 初始状态：[S' -> .S] 的闭包
    // 假设增广产生式 S' -> S 是 productions[0]
    LR0State initialState = calculateClosure({LR0Item(0, 0)});
    lr0States.push_back(initialState);

    map<LR0State, int> stateIndexMap; // 从状态 (项集)到其在lr0States中索引的映射
    stateIndexMap[initialState] = 0;

    queue<int> worklist; // 要处理的状态索引队列
    worklist.push(0);

    set<string> allSymbols; // 收集所有终结符和非终结符用于转换
    for(const auto& nt : nonTerminals) allSymbols.insert(nt);
    for(const auto& t : terminals) allSymbols.insert(t);
    // EPSILON 不应是DFA中的转换符号
    allSymbols.erase(EPSILON);

    while (!worklist.empty()) {
        int currentStateIdx = worklist.front();
        worklist.pop();
        const LR0State currentState = lr0States[currentStateIdx];

        for (const string& symbol : allSymbols) {
            // 跳过对 S' (如果不是原始开始符号) 或 EPSILON 的GOTO。
            if ((symbol == augmentedStartSymbol && symbol != originalStartSymbol) || symbol == EPSILON) {
                 continue;
            }

            LR0State nextStateCandidate = calculateGoto(currentState, symbol);
            if (!nextStateCandidate.empty()) {
                int nextStateIdx;
                if (stateIndexMap.find(nextStateCandidate) == stateIndexMap.end()) {
                    // 新状态
                    lr0States.push_back(nextStateCandidate);
                    nextStateIdx = lr0States.size() - 1;
                    stateIndexMap[nextStateCandidate] = nextStateIdx;
                    worklist.push(nextStateIdx);
                } else {
                    // 已有状态
                    nextStateIdx = stateIndexMap[nextStateCandidate];
                }
                lr0Transitions[currentStateIdx][symbol] = nextStateIdx;
            }
        }
    }
}

// LR(0) DFA Accessors
const vector<LR0State>& Grammar::getLR0States() const {
    return lr0States;
}

const map<int, map<string, int>>& Grammar::getLR0Transitions() const {
    return lr0Transitions;
}

const Production& Grammar::getProduction(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= productions.size()) {
        static Production errorProd;
        errorProd.lhs = "错误_无效索引";
        return errorProd;
    }
    return productions[index];
}

void Grammar::checkForSLR1Conflicts() {
    isSLR1ConflictFree_ = true;
    slrConflictMessages_.clear();

    if (lr0States.empty()) {
        if (errorMessage.empty()) {
            errorMessage = "无法检查SLR(1)冲突: LR(0) DFA不可用。";
        }
        isSLR1ConflictFree_ = false;
        slrConflictMessages_.push_back("LR(0) DFA无法构建。");
        return;
    }

    for (size_t i = 0; i < lr0States.size(); ++i) {
        const LR0State& state = lr0States[i];
        map<string, int> shiftActions; // 终结符 -> 目标状态索引
        vector<LR0Item> reduceItems;      // 类似 [A -> alpha .] 的项

        // 识别当前状态中的移进和归约项
        for (const LR0Item& item : state) {
            const Production& prod = productions[item.productionIndex];

            if (item.dotPosition < static_cast<int>(prod.rhs.size())) {
                const string& symbolAfterDot = prod.rhs[item.dotPosition];
                if (terminals.count(symbolAfterDot)) { // 仅在终结符上移进
                    if (lr0Transitions.count(i) && lr0Transitions.at(i).count(symbolAfterDot)) {
                         shiftActions[symbolAfterDot] = lr0Transitions.at(i).at(symbolAfterDot);
                    }
                }
            } else { // 归约项：[A -> alpha .] 或 [S' -> S .]
                reduceItems.push_back(item);
            }
        }

        // 1. 移进/归约冲突
        for (const auto& shiftAction : shiftActions) {
            const string& shiftTerminal = shiftAction.first;
            for (const LR0Item& reduceItem : reduceItems) {
                const Production& reduceProd = productions[reduceItem.productionIndex];
                if (reduceProd.isAugmented) continue; // S' -> S . 在 $ 上是接受，此处不是移进/归约冲突。

                if (followSets.count(reduceProd.lhs) && followSets.at(reduceProd.lhs).count(shiftTerminal)) {
                    isSLR1ConflictFree_ = false;
                    string conflictMsg = "状态 " + to_string(i) + ": 在终结符 '" + shiftTerminal + "' 上存在移进/归约冲突。\n";
                    
                    string specificShiftItemStr = "(无法确定具体的移进项)";
                    for(const LR0Item& potentialShiftItem : state) {
                        const Production& potShiftProd = productions[potentialShiftItem.productionIndex];
                        if (potentialShiftItem.dotPosition < static_cast<int>(potShiftProd.rhs.size()) && 
                            potShiftProd.rhs[potentialShiftItem.dotPosition] == shiftTerminal) {
                            specificShiftItemStr = potentialShiftItem.toString(productions);
                            break;
                        }
                    }
                    conflictMsg += "  移进行为: 基于项 [" + specificShiftItemStr + "] 对于终结符 '" + shiftTerminal + "' (到状态 " + to_string(shiftAction.second) + ")。\n";
                    conflictMsg += "  归约行为: 基于项 [" + reduceItem.toString(productions) + "] 因为 '" + shiftTerminal + "' 在 Follow(" + reduceProd.lhs + ") 中。";
                    slrConflictMessages_.push_back(conflictMsg);
                }
            }
        }

        // 2. 归约/归约冲突
        for (size_t r1_idx = 0; r1_idx < reduceItems.size(); ++r1_idx) {
            for (size_t r2_idx = r1_idx + 1; r2_idx < reduceItems.size(); ++r2_idx) {
                const LR0Item& item1 = reduceItems[r1_idx];
                const LR0Item& item2 = reduceItems[r2_idx];
                const Production& prod1 = productions[item1.productionIndex];
                const Production& prod2 = productions[item2.productionIndex];

                if (prod1.isAugmented || prod2.isAugmented) continue;
                if (item1.productionIndex == item2.productionIndex) continue; // Same rule.

                const auto& follow1 = followSets.at(prod1.lhs);
                const auto& follow2 = followSets.at(prod2.lhs);
                
                vector<string> commonFollowSymbols;
                set_intersection(follow1.begin(), follow1.end(),
                                      follow2.begin(), follow2.end(),
                                      back_inserter(commonFollowSymbols));

                if (!commonFollowSymbols.empty()) {
                    isSLR1ConflictFree_ = false;
                    string terminalsStr;
                    for(size_t k=0; k < commonFollowSymbols.size(); ++k) {
                        terminalsStr += "'" + commonFollowSymbols[k] + "'";
                        if (k < commonFollowSymbols.size() -1) terminalsStr += ", ";
                    }
                    string conflictMsg = "状态 " + to_string(i) + ": 规则 [" + item1.toString(productions) + "] 和规则 [" + item2.toString(productions) + "] 在共同的Follow符号上存在归约/归约冲突: { " + terminalsStr + " }.";
                    slrConflictMessages_.push_back(conflictMsg);
                }
            }
        }
    }
}

// LR(1) DFA 构建
LR1State Grammar::calculateLR1Closure(const LR1State& items) const {
    LR1State closureSet = items;
    queue<LR1Item> worklist;

    for (const auto& item : items) {
        worklist.push(item);
    }

    while (!worklist.empty()) {
        LR1Item currentItem = worklist.front();
        worklist.pop();

        const Production& prod = productions[currentItem.productionIndex];

        if (currentItem.dotPosition < static_cast<int>(prod.rhs.size())) {
            const string& symbolAfterDotB = prod.rhs[currentItem.dotPosition]; // 符号 B
            if (nonTerminals.count(symbolAfterDotB)) {
                // 项是 [A -> alpha . B beta, l]
                
                vector<string> betaSubsequence; // 序列 beta
                for (int k = currentItem.dotPosition + 1; k < static_cast<int>(prod.rhs.size()); ++k) {
                    betaSubsequence.push_back(prod.rhs[k]);
                }

                set<string> firstOfBeta = calculateFirstForSequence(betaSubsequence);
                
                set<string> newLookaheads;
                bool betaDerivesEpsilon = false;
                for (const string& term : firstOfBeta) {
                    if (term == EPSILON) {
                        betaDerivesEpsilon = true;
                    } else {
                        newLookaheads.insert(term);
                    }
                }
                if (betaDerivesEpsilon || betaSubsequence.empty()) {
                    newLookaheads.insert(currentItem.lookahead);
                }

                for (size_t prod_idx = 0; prod_idx < productions.size(); ++prod_idx) {
                    if (productions[prod_idx].lhs == symbolAfterDotB) { // 对于每个产生式 B -> gamma
                        for (const string& la : newLookaheads) {
                            if (la == EPSILON) continue; // 展望符必须是终结符或输入结束符。
                            LR1Item newItem(static_cast<int>(prod_idx), 0, la);
                            if (closureSet.find(newItem) == closureSet.end()) {
                                closureSet.insert(newItem);
                                worklist.push(newItem);
                            }
                        }
                    }
                }
            }
        }
    }
    return closureSet;
}

LR1State Grammar::calculateLR1Goto(const LR1State& items, const string& symbol) const {
    LR1State gotoSet;
    for (const auto& item : items) {
        const Production& prod = productions[item.productionIndex];
        if (item.dotPosition < static_cast<int>(prod.rhs.size())) {
            if (prod.rhs[item.dotPosition] == symbol) {
                gotoSet.insert(LR1Item(item.productionIndex, item.dotPosition + 1, item.lookahead));
            }
        }
    }
    return calculateLR1Closure(gotoSet);
}

void Grammar::buildLR1DFA() {
    lr1States_.clear();
    lr1Transitions_.clear();

    if (productions.empty() || productions[0].lhs != augmentedStartSymbol || productions[0].rhs.empty()) {
        if (errorMessage.empty()) errorMessage = "无法构建LR(1) DFA: 文法未增广或为空。";
        return;
    }

    // 初始状态：[S' -> .S, $] 的闭包
    // 增广产生式 S' -> S 是 productions[0]
    LR1State initialState = calculateLR1Closure({LR1Item(0, 0, END_OF_INPUT)});
    lr1States_.push_back(initialState);

    map<LR1State, int> stateIndexMap; 
    stateIndexMap[initialState] = 0;

    queue<int> worklist; 
    worklist.push(0);

    set<string> allSymbols; 
    for(const auto& nt : nonTerminals) allSymbols.insert(nt);
    for(const auto& t : terminals) allSymbols.insert(t);
    allSymbols.erase(EPSILON);
    // 如果 S' 与原始开始符号不同，则不计算其 GOTO。
    if (augmentedStartSymbol != originalStartSymbol) {
        allSymbols.erase(augmentedStartSymbol); 
    }

    while (!worklist.empty()) {
        int currentStateIdx = worklist.front();
        worklist.pop();
        const LR1State currentState = lr1States_[currentStateIdx];

        for (const string& symbol : allSymbols) {
            // 对 EPSILON 或 S' (如果不同) 的 GOTO 由 allSymbols 过滤处理。
            
            LR1State nextStateCandidate = calculateLR1Goto(currentState, symbol);
            if (!nextStateCandidate.empty()) {
                int nextStateIdx;
                if (stateIndexMap.find(nextStateCandidate) == stateIndexMap.end()) {
                    lr1States_.push_back(nextStateCandidate);
                    nextStateIdx = lr1States_.size() - 1;
                    stateIndexMap[nextStateCandidate] = nextStateIdx;
                    worklist.push(nextStateIdx);
                } else {
                    nextStateIdx = stateIndexMap[nextStateCandidate];
                }
                lr1Transitions_[currentStateIdx][symbol] = nextStateIdx;
            }
        }
    }
}

// LR(1) DFA Accessors
const vector<LR1State>& Grammar::getLR1States() const {
    return lr1States_;
}

const map<int, map<string, int>>& Grammar::getLR1Transitions() const {
    return lr1Transitions_;
}

void Grammar::buildLR1ParsingTable() {
    isLR1TableConflictFree_ = true;
    lr1TableConflictMessages_.clear();
    lr1ActionTable_.assign(lr1States_.size(), map<string, ParsingAction>());
    lr1GotoTable_.assign(lr1States_.size(), map<string, int>());

    if (lr1States_.empty()) {
        if (errorMessage.empty()) errorMessage = "无法构建LR(1)分析表: LR(1) DFA不可用。";
        isLR1TableConflictFree_ = false;
        lr1TableConflictMessages_.push_back("LR(1) DFA状态不可用。");
        return;
    }

    // 从 lr1Transitions_ 填充 GOTO 表 (针对非终结符)。
    for (size_t i = 0; i < lr1States_.size(); ++i) {
        if (lr1Transitions_.count(i)) {
            for (const auto& trans : lr1Transitions_.at(i)) {
                const string& symbol = trans.first;
                int targetState = trans.second;
                if (nonTerminals.count(symbol) && symbol != augmentedStartSymbol) {
                    lr1GotoTable_[i][symbol] = targetState;
                }
            }
        }
    }
    
    // 填充 ACTION 表。
    for (size_t i = 0; i < lr1States_.size(); ++i) {
        const LR1State& state = lr1States_[i];

        // 定义辅助函数来设置动作并处理冲突
        auto setAction = [&](const string& symbol, ParsingAction action) {
             if (lr1ActionTable_[i].count(symbol) && lr1ActionTable_[i][symbol].type != ParsingAction::Type::ERROR) {
                // 检测到冲突
                ParsingAction existing = lr1ActionTable_[i][symbol];
                bool resolved = false;

                // 解决策略 1: 移进 > 归约
                if (existing.type == ParsingAction::Type::SHIFT && action.type == ParsingAction::Type::REDUCE) {
                    // 保持移进 (existing)，忽略归约 (action)
                    resolved = true;
                    // 即使已解决也标记冲突
                    isLR1TableConflictFree_ = false;
                    string conflictMsg = "状态 " + to_string(i) + ", 符号 '" + symbol + "': 移进/归约冲突 (已解决: 保持移进) " + 
                                        existing.toString(productions) + " / " + action.toString(productions);
                    lr1TableConflictMessages_.push_back(conflictMsg);
                } else if (existing.type == ParsingAction::Type::REDUCE && action.type == ParsingAction::Type::SHIFT) {
                    // 将归约替换为移进
                    lr1ActionTable_[i][symbol] = action;
                    resolved = true;
                    // 标记冲突
                    isLR1TableConflictFree_ = false;
                    string conflictMsg = "状态 " + to_string(i) + ", 符号 '" + symbol + "': 移进/归约冲突 (已解决: 替换为移进) " + 
                                        existing.toString(productions) + " / " + action.toString(productions);
                    lr1TableConflictMessages_.push_back(conflictMsg);
                }
                // 解决策略 2: 归约/归约 - 选择较早的规则 (较小的产生式索引)
                else if (existing.type == ParsingAction::Type::REDUCE && action.type == ParsingAction::Type::REDUCE) {
                        if (action.value < existing.value) {
                            lr1ActionTable_[i][symbol] = action;
                        }
                        resolved = true;
                        // 标记冲突
                        isLR1TableConflictFree_ = false;
                        string conflictMsg = "状态 " + to_string(i) + ", 符号 '" + symbol + "': 归约/归约冲突 (已解决: 选择规则 " + 
                                            to_string(std::min(action.value, existing.value)) + ") " + 
                                            existing.toString(productions) + " / " + action.toString(productions);
                        lr1TableConflictMessages_.push_back(conflictMsg);
                }

                if (!resolved) {
                        isLR1TableConflictFree_ = false;
                        string conflictMsg = "状态 " + to_string(i) + ", 符号 '" + symbol + "': 冲突 " + 
                                            existing.toString(productions) + " / " + action.toString(productions);
                        lr1TableConflictMessages_.push_back(conflictMsg);
                        
                        action.type = ParsingAction::Type::CONFLICT;
                        action.conflictDetails = existing.toString(productions) + " / " + action.toString(productions);
                        lr1ActionTable_[i][symbol] = action;
                }
            } else {
                lr1ActionTable_[i][symbol] = action;
            }
        };

        // 从 lr1Transitions_ 获取移进动作 (针对终结符和 $)。
        if (lr1Transitions_.count(i)) {
            for (const auto& trans : lr1Transitions_.at(i)) {
                const string& symbol = trans.first;
                int targetState = trans.second;
                if (terminals.count(symbol) || symbol == END_OF_INPUT) {
                    setAction(symbol, ParsingAction(ParsingAction::Type::SHIFT, targetState));
                }
            }
        }

        // 从类似 [A -> alpha ., l] 的项获取归约和接受动作。
        for (const LR1Item& item : state) {
            // 检查项是否为归约项：点在末尾，或 [A -> . #, l] (点在0, RHS是EPSILON)
            bool isReduceItem = (item.dotPosition == static_cast<int>(productions[item.productionIndex].rhs.size()));
            if (!isReduceItem && productions[item.productionIndex].rhs.size() == 1 &&
                productions[item.productionIndex].rhs[0] == EPSILON && item.dotPosition == 0) {
                isReduceItem = true; // [A -> . #, l] 的特殊情况，等同于归约
            }

            if (isReduceItem) {
                const Production& rule = productions[item.productionIndex];
                const string& lookahead = item.lookahead;

                if (rule.isAugmented) { // [S' -> S ., $] 的接受动作
                    if (lookahead == END_OF_INPUT) {
                        setAction(END_OF_INPUT, ParsingAction(ParsingAction::Type::ACCEPT));
                    }
                } else { // [A -> alpha ., l], A != S' 的归约动作
                    setAction(lookahead, ParsingAction(ParsingAction::Type::REDUCE, item.productionIndex));
                }
            }
        }
    }
}
