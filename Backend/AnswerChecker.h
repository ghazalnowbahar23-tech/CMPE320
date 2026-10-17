
#ifndef ANSWERCHECKER_H
#define ANSWERCHECKER_H

#include "Flashcard.h"
#include <string>

using namespace std;

class AnswerChecker {
public:
    bool isCorrect(const string& userAnswer,
    const Flashcard& card) const;

private:
    string normalize(const string& answer) const;
};

#endif

