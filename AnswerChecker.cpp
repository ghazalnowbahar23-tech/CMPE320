
#include "AnswerChecker.h"
#include <algorithm>
#include <cctype>

using namespace std;

string AnswerChecker::normalize(const string& answer) const {
    string result = answer;

    // Convert all letters to lowercase.
    transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) {
            return static_cast<char>(tolower(c));
        });

    // Remove spaces from the beginning and end.
    const string whitespace = " \t\n\r";

    size_t first = result.find_first_not_of(whitespace);

    if (first == string::npos) {
        return "";
    }

    size_t last = result.find_last_not_of(whitespace);
    result = result.substr(first, last - first + 1);

    // Replace repeated spaces with a single space.
    string clean;
    bool previousWasSpace = false;

    for (char c : result) {
        if (isspace(static_cast<unsigned char>(c))) {
            if (!previousWasSpace) {
                clean += ' ';
            }

            previousWasSpace = true;
        } else {
            clean += c;
            previousWasSpace = false;
        }
    }

    return clean;
}

bool AnswerChecker::isCorrect(const string& userAnswer,
     const Flashcard& card) const {
    string normalizedUserAnswer = normalize(userAnswer);

    if (normalizedUserAnswer.empty()) {
        return false;
    }

    for (const string& acceptedAnswer : card.acceptedAnswers) {
        if (normalizedUserAnswer == normalize(acceptedAnswer)) {
            return true;
        }
    }

    return false;
}


