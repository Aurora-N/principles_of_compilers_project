#ifndef CLASSIFIER_H
#define CLASSIFIER_H
#include <string>

// 定义异常
class ParseException {
public:
    ParseException(const std::string& msg) : message(msg) {}
    std::string message;
};

std::string classifier(std::string input_text);
#endif // CLASSIFIER_H
