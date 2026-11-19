
#ifndef ANSWERCHECKER_H
#define ANSWERCHECKER_H

#include "Flashcard.h"
#include <string>

class AnswerChecker {
public:
    bool isCorrect(const std::string& userAnswer,
    const Flashcard& card) const;

private:
    std::string normalize(const std::string& answer) const;
};

#endif

