#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <QString>
#include <QStringList>

using namespace std;

// 产生式: Lhs -> Rhs
struct Production {
    string lhs;
    vector<string> rhs;
    int originalRuleIndex; // 用户定义的文法索引
    bool isAugmented;      // 标记是否为扩充文法

    Production() : originalRuleIndex(-1), isAugmented(false) {} // 默认构造函数
    Production(string l, vector<string> r, int idx, bool aug = false)
        : lhs(move(l)), rhs(move(r)), originalRuleIndex(idx), isAugmented(aug) {}
};

// LR(0)项: [A -> alpha . beta]
struct LR0Item {
    int productionIndex; // 文法产生式vector索引
    int dotPosition;

    LR0Item(int prodIdx, int dotPos) : productionIndex(prodIdx), dotPosition(dotPos) {}

    // 用于set和map比较
    bool operator<(const LR0Item& other) const {
        if (productionIndex != other.productionIndex) {
            return productionIndex < other.productionIndex;
        }
        return dotPosition < other.dotPosition;
    }
    bool operator==(const LR0Item& other) const {
        return productionIndex == other.productionIndex && dotPosition == other.dotPosition;
    }

    // 获取字符串表示
    string toString(const vector<Production>& allProductions) const;
};

// LR(0)状态，是LR(0)项的集合
using LR0State = set<LR0Item>;

// LR(1)项: [A -> alpha . beta, lookahead]
struct LR1Item {
    int productionIndex;
    int dotPosition;
    string lookahead; // 向前查看的符号

    LR1Item(int prodIdx, int dotPos, string la) 
        : productionIndex(prodIdx), dotPosition(dotPos), lookahead(move(la)) {}

    bool operator<(const LR1Item& other) const {
        if (productionIndex != other.productionIndex) return productionIndex < other.productionIndex;
        if (dotPosition != other.dotPosition) return dotPosition < other.dotPosition;
        return lookahead < other.lookahead;
    }
    bool operator==(const LR1Item& other) const {
        return productionIndex == other.productionIndex && 
               dotPosition == other.dotPosition && 
               lookahead == other.lookahead;
    }

    string toString(const vector<Production>& allProductions) const;
};

// LR(1)状态，是LR(1)项的集合
using LR1State = set<LR1Item>;

// LR解析表中的Action
struct ParsingAction {
    enum class Type { SHIFT, REDUCE, ACCEPT, ERROR, CONFLICT };
    Type type = Type::ERROR;
    int value = -1; // 用于SHIFT的state索引，用于REDUCE的产生式索引
    string conflictDetails; // 如果type是CONFLICT，存储详细信息

    ParsingAction(Type t = Type::ERROR, int v = -1, string cd = "") : type(t), value(v), conflictDetails(move(cd)) {}

    string toString(const vector<Production>& allProductions) const;
};

class Grammar {
public:
    Grammar();

    // 解析文法输入字符串
    // 错误信息可以通过getErrorMessage()获取
    bool parseGrammar(const QString& grammarInput);

    // 获取First集
    const map<string, set<string>>& getFirstSets() const;
    // 获取Follow集
    const map<string, set<string>>& getFollowSets() const;
    // 获取所有产生式
    const vector<Production>& getProductions() const;
    // 获取所有非终结符
    const set<string>& getNonTerminals() const;
    // 获取所有终结符
    const set<string>& getTerminals() const;
    // 获取开始符号
    const string& getStartSymbol() const;
    // 获取错误信息
    string getErrorMessage() const;

    // LR(0) DFA
    const vector<LR0State>& getLR0States() const;
    const map<int, map<string, int>>& getLR0Transitions() const; 
    // 获取产生式
    const Production& getProduction(int index) const;

    // SLR(1)状态
    bool isSLR1ConflictFree() const { return isSLR1ConflictFree_; }
    // 获取SLR(1)冲突信息
    const vector<string>& getSLRConflictMessages() const { return slrConflictMessages_; }

    // LR(1) DFA
    const vector<LR1State>& getLR1States() const;
    // 获取LR(1)状态转移
    const map<int, map<string, int>>& getLR1Transitions() const;

    // LR(1)解析表
    const vector<map<string, ParsingAction>>& getLR1ActionTable() const { return lr1ActionTable_; }
    // 获取LR(1)状态转移
    const vector<map<string, int>>& getLR1GotoTable() const { return lr1GotoTable_; }
    // 获取LR(1)冲突信息
    bool isLR1ConflictFree() const { return isLR1TableConflictFree_; }
    const vector<string>& getLR1TableConflictMessages() const { return lr1TableConflictMessages_; }

    // 文法处理中使用的常量
    static const string EPSILON;
    static const string END_OF_INPUT;
    static const string AUGMENTED_START_SYMBOL;

private:
    // 重置所有内部文法数据结构
    void clear();

    // 解析阶段1: 识别所有非终结符并确定开始符号
    bool identifyNonTerminalsAndStartSymbol(const QStringList& lines);

    // 解析阶段2: 解析每个产生式规则
    bool parseProductionsAndTerminals(const QStringList& lines);

    // 将产生式右部（RHS）分解为符号
    vector<string> tokenizeRHS(const string& rhsString, int ruleIndexForError);

    // 计算所有非终结符的First集
    void calculateFirstSets();

    // 计算所有非终结符的Follow集
    void calculateFollowSets();

    // 计算First集的辅助函数: 计算符号序列的First集
    set<string> calculateFirstForSequence(const vector<string>& sequence) const;

    // LR(0) DFA构造函数
    void buildLR0DFA();
    LR0State calculateClosure(const LR0State& items) const;
    LR0State calculateGoto(const LR0State& items, const string& symbol) const;

    void augmentGrammar(); // 添加S' -> S规则
    void checkForSLR1Conflicts(); // 检查SLR(1)冲突

    // LR(1) DFA构造函数
    void buildLR1DFA();
    LR1State calculateLR1Closure(const LR1State& items) const;
    LR1State calculateLR1Goto(const LR1State& items, const string& symbol) const;

    // LR(1)解析表构造函数
    void buildLR1ParsingTable();

    string originalStartSymbol; // 存储原始开始符号
    string startSymbol;
    set<string> nonTerminals;
    set<string> terminals;           // 不包括EPSILON
    vector<Production> productions; // 存储所有单独的产生式（例如，A->BC, A->D）

    map<string, set<string>> firstSets;
    map<string, set<string>> followSets;

    string errorMessage; // 存储错误信息

    // LR(0) DFA数据
    vector<LR0State> lr0States;                      // LR(0)项集合
    map<int, map<string, int>> lr0Transitions;       // DFA转移: state_index -> (symbol -> state_index)
    string augmentedStartSymbol;

    // SLR(1)数据
    bool isSLR1ConflictFree_;
    vector<string> slrConflictMessages_;

    // LR(1) DFA数据
    vector<LR1State> lr1States_;
    map<int, map<string, int>> lr1Transitions_;

    // LR(1)解析表数据
    vector<map<string, ParsingAction>> lr1ActionTable_;
    vector<map<string, int>> lr1GotoTable_;
    bool isLR1TableConflictFree_;
    vector<string> lr1TableConflictMessages_;
};

#endif // GRAMMAR_H
