#include "StudyTime.h"

using namespace std;
 // starting the game with the given flashcard and reset all progresses
 StudyTime:: StudyTime(const vector<Flashcard>& deck) 
 : cards(deck),
currentCardIndex(0),
score(0),
correct(0),
incorrect(0),
characterStep(0)
{
}
// return the current flashcard or a null pointer if the game is finished
const Flashcard* StudyTime::getCurrentCard() const{
    if(isFinished()){
        return nullptr;
    }
       return &cards[currentCardIndex];
}
// check answer and update the score, character position and streak
bool StudyTime:: submitAnswer(const string& userAnswer) {
    if (isFinished()){
        return false;
    }
    const Flashcard& currentCard = cards[currentCardIndex];
    if (answerChecker.isCorrect(userAnswer, currentCard)){
        correct++;
        score += 5;
        currentStreak++;
        characterStep++;
    }else{
        incorrect++ ;
        currentStreak = 0;

        if(characterStep> 0) {
            characterStep--;
        }
    }
    currentCardIndex ++;
    return true;
}
// return the current game progress and statistics
int StudyTime:: getScore() const{
    return score;
}
int StudyTime::getCorrect() const{
    return correct;
}
int StudyTime:: getIncorrect()const {
    return incorrect;
}
int StudyTime:: getCurrentStreak()const {
    return currentStreak;
}
int StudyTime:: getCharacterStep()const {
    return characterStep;
}
int StudyTime:: getCurrentCardIndex()const {
    return currentCardIndex;
}
//check if all the cards are answered
bool StudyTime ::isFinished() const{
    return currentCardIndex >= static_cast<int>(cards.size());
}
