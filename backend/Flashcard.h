#ifndef Flashcard_h
#define Flashcard_h

#include <string>
#include<vector>

struct Flashcard{
    int id;
    std::string question;
    std::vector<std::string> acceptedAnswers;
};

#endif
