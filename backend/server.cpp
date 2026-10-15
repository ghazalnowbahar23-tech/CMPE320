#include <crow.h>
#include "StudyTime.h"
#include <vector>

using namespace std;

int main() {
    crow::SimpleApp app;

    // Temporary flashcards for testing the server.
    vector<Flashcard> deck = {
        {1, "What is 2 + 2?", {"4"}},
        {2, "What is the capital of France?", {"Paris"}},
        {3, "What language are we using?", {"C++"}}
    };

    StudyTime session(deck);

    // Send the current question to the browser.
    CROW_ROUTE(app, "/api/question")
    ([&session]() {
        crow::json::wvalue result;

        const Flashcard* card = session.getCurrentCard();

        if (card == nullptr) {
            result["finished"] = true;
        } else {
            result["finished"] = false;
            result["id"] = card->id;
            result["question"] = card->question;
        }

        return crow::response(result);
    });

    // Receive an answer and return the updated game statistics.
    CROW_ROUTE(app, "/api/answer")
        .methods(crow::HTTPMethod::POST)
    ([&session](const crow::request& req) {
        auto data = crow::json::load(req.body);

        if (!data || !data.has("answer") ||
            data["answer"].t() != crow::json::type::String) {
            return crow::response(400, "Please provide an answer.");
        }

        if (session.isFinished()) {
            return crow::response(400, "The game has finished.");
        }

        string answer = data["answer"].s();

        int previousCorrect = session.getCorrect();
        session.submitAnswer(answer);
        bool correct = session.getCorrect() > previousCorrect;

        crow::json::wvalue result;
        result["correct"] = correct;
        result["score"] = session.getScore();
        result["correctCount"] = session.getCorrect();
        result["incorrectCount"] = session.getIncorrect();
        result["streak"] = session.getCurrentStreak();
        result["characterStep"] = session.getCharacterStep();
        result["finished"] = session.isFinished();

        return crow::response(result);
    });

    app.port(18080).run();

    return 0;
}
