
#include <iostream>
#include "FlashcardManager.h"

using namespace std;

int main() {
    FlashcardManager manager;

    // Check the initial number of cards
    cout << "Initial cards: " << manager.getAllCards().size() << endl;

    // Add a new flashcard
    int newId = manager.addCard("What is 5 + 5?", {"10"});
    cout << "New card ID: " << newId << endl;

    // Find the card we created
    const Flashcard* card = manager.getCardById(newId);

    if (card != nullptr) {
        cout << "Question: " << card->question << endl;
    }

    // Change the question and its answer
    bool updated = manager.updateCard(newId, "What's 5 - 5?", {"0"});
    cout << "Updated: " << (updated ? "Yes" : "No") << endl;

    // Delete the card
    bool deleted = manager.deleteCard(newId);
    cout << "Deleted: " << (deleted ? "Yes" : "No") << endl;

    // Confirm the card count returns to its original value
    cout << "Final cards: " << manager.getAllCards().size() << endl;

    return 0;
}
