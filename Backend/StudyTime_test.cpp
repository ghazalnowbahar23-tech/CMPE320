#include "StudyTime.h"
#include <iostream>
using namespace std;

int main(){
    //small deck just for testing
    vector<Flashcard> deck= {
        {1, "What is 2 + 2?", {"4"}},
        {2, "What is my name", {"ghazal"}},
        {3, "What language are we using?", {"C++"}}
    };
    // name game
    StudyTime session(deck);
    cout<< "== Card Climb test==" <<endl;
    //checl the first question
    const Flashcard* card = session.getCurrentCard();
    if (card != nullptr){
        cout<< "Question:" << card->question<< endl;
    }

//submit a correct answer
session.submitAnswer("4");
cout<< "\n After first answer:"<< endl;
cout<< "Score:"<< session.getScore() <<endl;
cout<<" Streak:"<<session.getCurrentStreak() <<endl;
cout<<"Character step:" <<session.getCharacterStep() <<endl;

// lets try a wrong answer
session.submitAnswer("londonn");

cout << "\nAfter second answer:" << endl;
cout << "Score: " << session.getScore() << endl;
cout << "Incorrect: " << session.getIncorrect() << endl;
cout << "streak: " << session.getCurrentStreak() << endl;
cout << "Character step: " << session.getCharacterStep() << endl;

//answer correct again
session.submitAnswer("C++");
cout << "\nAfter third answer:" << endl;
    cout << "Score: " << session.getScore() << endl;
    cout << "Correct: " << session.getCorrect() << endl;
    cout << "streak: " << session.getCurrentStreak() << endl;
    cout << "Character step: " << session.getCharacterStep() << endl;
    cout << "Game finished: " << (session.isFinished() ? "Yes" : "No") << endl;
return 0;
}