#ifndef Flashcard_h
#define Flashcard_h

#include <string>
#include<vector>

using namespace std;

struct Flashcard{
    int id;
    string question;
    vector<string> acceptedAnswers;
};

#endif
