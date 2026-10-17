#include "FlashcardManager.h"

using namespace std;

//three samble flashcards
FlashcardManager::FlashcardManager()
: cards({
    {1, "1+1?", {"2"}},
    {2, "What is my name?", {"Ghazal"}},
    {3, "What language are we using?", {"C++"}}
}),
nextId(4){}

//returns the complete list of cards
const vector<Flashcard>& FlashcardManager::getAllCards() const {
    return cards;
}
const Flashcard* FlashcardManager:: getCardById(int id) const{
    for (const Flashcard& card : cards){
        if(card.id == id){
            return &card;
        }
    }
    return nullptr;
}
//creates a card if its question and answers are acceptable
int FlashcardManager::addCard(
    const string& question, const vector<string>& answers){
        if (question.empty() || answers.empty()){
            return -1; //not accepted
        }
        for (const string& answer : answers){
            if(answer.empty()){
                return -1; //all the accepted answers MUST contain text

            }
        }   
        int id = nextId ++; // assign an ID to this card
        cards.push_back({id,question,answers});
        return id;
     }

     //this changes the question and answers of an existing card
     bool FlashcardManager :: updateCard( int id, const string& question, const vector<string>& answers){
        if(question.empty() || answers.empty()){
            return false;
        }
        for (const string& answer : answers) {
        if (answer.empty()) {
            return false;
        }
        }
        for(Flashcard& card: cards){
            if(card.id == id){
                card.question = question;
                card.acceptedAnswers = answers;
                return true; // successfully updated
            }
        }
        return false; // no card with this id was found

     }
     //now we remove a card from the list
bool FlashcardManager ::deleteCard(int id){
    for( auto i= cards.begin(); i != cards.end(); ++i) {
        if(i->id == id){
            cards.erase(i);
            return true; //deletion successful
        }
    }
return false; //no card with this id was found
}


