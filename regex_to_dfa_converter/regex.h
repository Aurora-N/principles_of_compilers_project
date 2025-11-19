#ifndef REGEX_H
#define REGEX_H
#include <string>
#include "nfa.h"

// 基础数据结构定义
struct Regex {
    std::string id; // 正则表达式的名称，如digit
    std::string rawRegex;   // 用户输入的所有内容，包含id，如digit=[0-3]
    std::string rawContent; // 用户输入的正则表达式原始内容，如[0-3]
    std::string content; // 经过初步解析后的正则表达式内容，如(0|1|2|3)
    NFA nfa; // 这个正则表达式的NFA
    DFA dfa; // 这个正则表达式的DFA
    DFA minDfa; // 这个正则表达式的最小化DFA

    Regex(std::string inputContent, std::string id="");
};

void preprocessRegex(Regex &regex);

void initNFA(Regex &regex);

void initDFA(Regex &regex);

void initMinDFA(Regex &regex);
#endif // REGEX_H
