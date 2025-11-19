#include "nfa.h"
using namespace std;


string stateToString(std::set<int> target)
{
    if (target.size() == 1) {
        return std::to_string(*target.rbegin());
    }
    string result = "{ ";

    for (auto it = target.begin(); it != target.end(); ++it) {
        result += std::to_string(*it);
        if (std::next(it) != target.end()) {
            result += ", ";
        }
    }

    result += " }";
    return result;
}
