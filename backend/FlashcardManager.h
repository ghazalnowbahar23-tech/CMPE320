#ifndef FLASHCARDMANAGER_H
#define FLASHCARDMANAGER_H

#include "Flashcard.h"
#include <vector>
#include <string>

class FlashcardManager {
    public:
    FlashcardManager();
    //returns all flashcards
    const vector<Flashcard>& getAllCards() const;

    //finds a card using its id
    const Flashcard* getCardById (int id) const;

    //adds a new cards and returns its id
    int addCard( const string& question, const vector<string>& answers);
    
    // updates an exisiting card
    bool updateCard(int id, const string& question, const vector<string>& answers);
    // deletes a card using its id
    bool deleteCard(int id);

private:
vector<Flashcard> cards; // stores all the cards
int nextId; // id assigned to the next card
};
#endif