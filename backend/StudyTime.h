#ifndef StudyTime_H
#define StudyTime_H

#include "Flashcard.h"
#include "AnswerChecker.h"
#include "UserData.h"
#include <vector>
#include <string>

// This class creates a game using a deck of cards
class StudyTime{
     public:
     StudyTime(const std::vector<Flashcard>& deck);
     // presents the current question being asked
     const Flashcard* getCurrentCard() const;
    
     //submits the answer given by the user for that specific question
bool submitAnswer (const std::string& userAnswer);

// Game progress 
int getCurrentCardIndex() const;
int getScore() const;
int getCorrect() const;
int getIncorrect() const;
int getCurrentStreak()const;
int getCharacterStep() const;

// tells the user if the game is finished
bool isFinished() const;

private:
std::vector<Flashcard> cards;
std::vector<StudyRecord> studyRecords;
AnswerChecker answerChecker;


int currentCardIndex;
int score;
int correct;
int incorrect;
int currentStreak;
int characterStep;

};


#endif
