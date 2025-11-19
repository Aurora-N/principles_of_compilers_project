#ifndef NFA_H
#define NFA_H
#include <set>
#include <string>
#include <vector>

// NFA中状态转移定义
struct Transition {
    int from;
    int to;
    std::string symbol;  // 使用#表示epsilon转移

    Transition(int from, int to, std::string symbol = "#") : from(from), to(to), symbol(symbol) {}
};

// DFA中状态转移定义
struct DFATransition {
    std::set<int> from;
    std::set<int> to;
    std::string symbol;  // 使用#表示epsilon转移

    DFATransition(int from, int to, std::string symbol = "#") : symbol(symbol) {
        this->from.insert(from);
        this->to.insert(to);
    }
    DFATransition(std::set<int> from, std::set<int> to, std::string symbol = "#") : from(from), to(to), symbol(symbol) {}
};

// NFA集合定义
struct NFA {
    int startState; // 开始状态
    std::set<int> acceptStates; // 结束状态（可能有多个）
    std::vector<Transition> transitions; // 状态转移存储
    int stateCounter; // 最大状态编号
};

// DFA定义
struct DFA {
    std::set<int> startState; // 开始状态
    std::set<std::set<int> > acceptStates; // 结束状态（可能有多个）
    std::vector<DFATransition> transitions; // 状态转移存储
    int stateCounter; // 最大状态编号
};

std::string stateToString(std::set<int> target);
#endif // NFA_H
