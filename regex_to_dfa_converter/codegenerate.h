#ifndef CODEGENERATE_H
#define CODEGENERATE_H
#include <QString>
#include "regex.h"

QString generateIdPredicate(const Regex& idRegex);
const Regex* findIdRegex(const QString& id, QList<Regex> regexs);
QString generateLexerCodeForRegex(const Regex& regex, QList<Regex> regexs);
#endif // CODEGENERATE_H
