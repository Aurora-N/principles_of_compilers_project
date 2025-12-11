#ifndef CODEGENERATE_H
#define CODEGENERATE_H
#include <QString>
#include "regex.h"

QString generateLexerCode(QList<Regex> regexs);
#endif // CODEGENERATE_H
