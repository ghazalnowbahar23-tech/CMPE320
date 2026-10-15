#include <iostream>
#include <string>

#include "Flashcard.h"
#include "AnswerChecker.h"

using namespace std;

int main() {
    Flashcard card{
        1,
        "What is the name of the CMPE320 Professor?",
        {"Samir"}
    };

    AnswerChecker checker;
    string answer;
    cout<< card.question <<endl;
    cout<< "Type in your answer";
    getline(cin, answer);

   if (checker.isCorrect(answer, card)){
    cout << "Correct! you climb one step"<< endl;
   } else{
    cout<< "Wrong, you fall down one steo"<< endl;
   }
   cout << "moving to the next card"<< endl;
  
   return 0;

}

